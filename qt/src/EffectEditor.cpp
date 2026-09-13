#include "EffectEditor.h"

#include "Patch.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <functional>
#include <string_view>
#include <utility>

class ParamRow : public QWidget
{
public:
  ParamRow(const ParamSpec& spec,
           std::function<void(const ParamSpec&, int)> onEdited,
           QWidget* parent = nullptr)
      : QWidget(parent)
      , spec_(spec)
      , onEdited_(std::move(onEdited))
  {
    setObjectName(QStringLiteral("param-%1").arg(spec_.offset));
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    if (spec_.type == ParamType::Checkbox) {
      check_ = new QCheckBox(this);
      check_->setText(QString::fromUtf8(spec_.label));
      layout->addWidget(check_);
      layout->addStretch(1);
      connect(check_, &QCheckBox::toggled, this, [this](bool on) { emitRaw(on ? 1 : 0); });
      return;
    }

    if (spec_.type == ParamType::Combo) {
      combo_ = new QComboBox(this);
      for (int i = 0; i < spec_.comboCount; ++i)
        combo_->addItem(QString::fromUtf8(spec_.comboItems[static_cast<std::size_t>(i)]));
      combo_->setMinimumContentsLength(8);
      layout->addWidget(combo_, 1);
      connect(combo_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        emitRaw(std::clamp(index, spec_.min, spec_.max));
      });
      return;
    }

    slider_ = new QSlider(Qt::Horizontal, this);
    slider_->setRange(spec_.min, spec_.max);
    slider_->setPageStep(std::max(1, (spec_.max - spec_.min) / 10));
    slider_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    layout->addWidget(slider_, 1);

    if (usesDoubleSpin()) {
      dspin_ = new QDoubleSpinBox(this);
      configureDoubleSpin();
      layout->addWidget(dspin_);
      connect(slider_, &QSlider::valueChanged, this, [this](int raw) {
        QSignalBlocker blocker(dspin_);
        dspin_->setValue(rawToDisplay(spec_, raw));
        emitRaw(raw);
      });
      connect(dspin_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double display) {
        const int raw = displayToRaw(spec_, display);
        QSignalBlocker blocker(slider_);
        slider_->setValue(raw);
        emitRaw(raw);
      });
    } else {
      spin_ = new QSpinBox(this);
      configureIntSpin();
      layout->addWidget(spin_);
      connect(slider_, &QSlider::valueChanged, this, [this](int raw) {
        const int display = static_cast<int>(std::lround(rawToDisplay(spec_, raw)));
        QSignalBlocker blocker(spin_);
        spin_->setValue(display);
        emitRaw(raw);
      });
      connect(spin_, qOverload<int>(&QSpinBox::valueChanged), this, [this](int display) {
        const int raw = displayToRaw(spec_, static_cast<double>(display));
        QSignalBlocker blocker(slider_);
        slider_->setValue(raw);
        emitRaw(raw);
      });
    }
  }

  [[nodiscard]] const ParamSpec& spec() const { return spec_; }

  [[nodiscard]] QString labelText() const { return QString::fromUtf8(spec_.label); }

  [[nodiscard]] int rawValue() const
  {
    if (check_)
      return check_->isChecked() ? 1 : 0;
    if (combo_)
      return std::clamp(combo_->currentIndex(), spec_.min, spec_.max);
    if (slider_)
      return std::clamp(slider_->value(), spec_.min, spec_.max);
    return spec_.min;
  }

  void setRawValue(int raw, bool notify)
  {
    raw = std::clamp(raw, spec_.min, spec_.max);
    if (check_) {
      QSignalBlocker blocker(check_);
      check_->setChecked(raw != 0);
    } else if (combo_) {
      QSignalBlocker blocker(combo_);
      combo_->setCurrentIndex(raw);
    } else if (slider_) {
      QSignalBlocker blocker(slider_);
      slider_->setValue(raw);
      if (spin_) {
        QSignalBlocker spinBlocker(spin_);
        spin_->setValue(static_cast<int>(std::lround(rawToDisplay(spec_, raw))));
      }
      if (dspin_) {
        QSignalBlocker spinBlocker(dspin_);
        dspin_->setValue(rawToDisplay(spec_, raw));
      }
    }
    if (notify)
      emitRaw(raw);
  }

  void loadFrom(const Patch& patch) { setRawValue(readParam(patch, spec_), false); }

private:
  [[nodiscard]] bool usesDoubleSpin() const
  {
    return spec_.transform == DisplayTransform::LevelDb
           || spec_.transform == DisplayTransform::QValue
           || spec_.transform == DisplayTransform::FreqLinear
           || spec_.transform == DisplayTransform::FreqLog;
  }

  void configureIntSpin()
  {
    const int displayMin = static_cast<int>(std::lround(rawToDisplay(spec_, spec_.min)));
    const int displayMax = static_cast<int>(std::lround(rawToDisplay(spec_, spec_.max)));
    spin_->setRange(std::min(displayMin, displayMax), std::max(displayMin, displayMax));
    spin_->setKeyboardTracking(false);
    spin_->setAccelerated(true);
    spin_->setAlignment(Qt::AlignRight);
    spin_->setMinimumWidth(72);
    if (spec_.suffix)
      spin_->setSuffix(QString::fromUtf8(spec_.suffix));
  }

  void configureDoubleSpin()
  {
    dspin_->setRange(rawToDisplay(spec_, spec_.min), rawToDisplay(spec_, spec_.max));
    dspin_->setKeyboardTracking(false);
    dspin_->setAccelerated(true);
    dspin_->setAlignment(Qt::AlignRight);
    dspin_->setMinimumWidth(80);
    if (spec_.transform == DisplayTransform::FreqLinear
        || spec_.transform == DisplayTransform::FreqLog) {
      dspin_->setDecimals(0);
      dspin_->setSingleStep(1.0);
      dspin_->setMinimumWidth(88);
    } else {
      dspin_->setDecimals(1);
      dspin_->setSingleStep(spec_.transform == DisplayTransform::LevelDb ? 0.5 : 0.1);
    }
    if (spec_.suffix)
      dspin_->setSuffix(QString::fromUtf8(spec_.suffix));
  }

  void emitRaw(int raw)
  {
    if (onEdited_)
      onEdited_(spec_, std::clamp(raw, spec_.min, spec_.max));
  }

  ParamSpec spec_{};
  std::function<void(const ParamSpec&, int)> onEdited_;
  QSlider* slider_ = nullptr;
  QSpinBox* spin_ = nullptr;
  QDoubleSpinBox* dspin_ = nullptr;
  QComboBox* combo_ = nullptr;
  QCheckBox* check_ = nullptr;
};

namespace {

std::vector<const char*> uniqueGroups(std::span<const ParamSpec> params)
{
  std::vector<const char*> groups;
  for (const auto& param : params) {
    if (!param.group)
      continue;
    const bool seen = std::any_of(groups.begin(), groups.end(), [&](const char* g) {
      return std::string_view(g) == param.group;
    });
    if (!seen)
      groups.push_back(param.group);
  }
  return groups;
}

QScrollArea* wrapScroll(QWidget* inner, QWidget* parent)
{
  auto* scroll = new QScrollArea(parent);
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  scroll->setWidget(inner);
  return scroll;
}

} // namespace

EffectEditor::EffectEditor(QWidget* parent)
    : QWidget(parent)
{
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  title_ = new QLabel(this);
  auto titleFont = title_->font();
  titleFont.setBold(true);
  titleFont.setPointSizeF(titleFont.pointSizeF() + 1);
  title_->setFont(titleFont);
  layout->addWidget(title_);

  stack_ = new QStackedWidget(this);
  layout->addWidget(stack_, 1);

  rebuildPages();
  reload();
}

void EffectEditor::rebuildPages()
{
  auto* empty = new QLabel(QStringLiteral("Select a chip in the signal chain"), stack_);
  empty->setAlignment(Qt::AlignCenter);
  empty->setEnabled(false);
  stack_->addWidget(empty);

  for (const auto& spec : allEffectSpecs())
    stack_->addWidget(buildPage(spec));
}

QWidget* EffectEditor::buildPage(const EffectSpec& spec)
{
  auto onEdited = [this](const ParamSpec& specRef, int raw) {
    if (!patch_)
      return;
    writeParam(*patch_, specRef, raw);
    emit parameterEdited(specRef.offset, specRef.byteWidth, raw);
  };

  auto* inner = new QWidget;
  auto& dest = rows_[static_cast<std::size_t>(spec.kind)];
  dest.clear();

  auto makeRow = [&](const ParamSpec& param, QWidget* parent) {
    auto* row = new ParamRow(param, onEdited, parent);
    dest.push_back(row);
    return row;
  };

  if (spec.layout == LayoutKind::EqBands) {
    auto* columns = new QHBoxLayout(inner);
    columns->setContentsMargins(0, 0, 0, 0);
    columns->setSpacing(8);
    for (const char* group : uniqueGroups(spec.params)) {
      auto* box = new QGroupBox(QString::fromUtf8(group), inner);
      auto* form = new QFormLayout(box);
      form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
      form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
      for (const auto& param : spec.params) {
        if (!param.group || std::string_view(param.group) != group)
          continue;
        form->addRow(QString::fromUtf8(param.label), makeRow(param, box));
      }
      columns->addWidget(box, 1);
    }
    columns->addStretch(0);
    return wrapScroll(inner, stack_);
  }

  if (spec.layout == LayoutKind::TapRows) {
    auto* rows = new QVBoxLayout(inner);
    rows->setContentsMargins(0, 0, 0, 0);
    rows->setSpacing(8);
    for (const char* group : uniqueGroups(spec.params)) {
      auto* box = new QGroupBox(QString::fromUtf8(group), inner);
      auto* form = new QHBoxLayout(box);
      form->setContentsMargins(8, 8, 8, 8);
      form->setSpacing(12);
      for (const auto& param : spec.params) {
        if (!param.group || std::string_view(param.group) != group)
          continue;
        auto* field = new QWidget(box);
        auto* fieldLayout = new QVBoxLayout(field);
        fieldLayout->setContentsMargins(0, 0, 0, 0);
        fieldLayout->setSpacing(2);
        auto* label = new QLabel(QString::fromUtf8(param.label), field);
        fieldLayout->addWidget(label);
        fieldLayout->addWidget(makeRow(param, field));
        form->addWidget(field, 1);
      }
      rows->addWidget(box);
    }
    rows->addStretch(1);
    return wrapScroll(inner, stack_);
  }

  auto* form = new QFormLayout(inner);
  form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
  form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
  form->setContentsMargins(0, 0, 8, 0);
  for (const auto& param : spec.params) {
    auto* row = makeRow(param, inner);
    if (param.type == ParamType::Checkbox)
      form->addRow(row);
    else
      form->addRow(QString::fromUtf8(param.label), row);
  }
  return wrapScroll(inner, stack_);
}

void EffectEditor::setPatch(Patch* patch)
{
  patch_ = patch;
  reload();
}

void EffectEditor::setIdentity(int identity)
{
  identity_ = identity;
  reload();
}

void EffectEditor::refresh()
{
  reload();
}

QString EffectEditor::titleText() const
{
  return title_ ? title_->text() : QString();
}

int EffectEditor::paramCount() const
{
  return static_cast<int>(currentRows().size());
}

QString EffectEditor::paramLabel(int index) const
{
  auto* row = rowAt(index);
  return row ? row->labelText() : QString();
}

int EffectEditor::paramOffset(int index) const
{
  auto* row = rowAt(index);
  return row ? row->spec().offset : -1;
}

int EffectEditor::paramRawValue(int index) const
{
  auto* row = rowAt(index);
  return row ? row->rawValue() : 0;
}

void EffectEditor::setParamRawValue(int index, int raw)
{
  auto* row = rowAt(index);
  if (!row)
    return;
  row->setRawValue(raw, true);
}

void EffectEditor::reload()
{
  kind_ = EffectKind::Count;
  if (!patch_ || identity_ < 0 || identity_ >= Patch::kEffectCount) {
    title_->clear();
    title_->setVisible(false);
    stack_->setCurrentIndex(0);
    return;
  }

  kind_ = kindForSlot(identity_, patch_->blockB2Mode(), patch_->isDistortion());
  if (kind_ == EffectKind::Count) {
    title_->clear();
    title_->setVisible(false);
    stack_->setCurrentIndex(0);
    return;
  }

  const auto& spec = specFor(kind_);
  title_->setText(QString::fromUtf8(spec.name));
  title_->setVisible(true);
  stack_->setCurrentIndex(1 + static_cast<int>(kind_));
  loadCurrentPage();
}

void EffectEditor::loadCurrentPage()
{
  if (!patch_ || kind_ == EffectKind::Count)
    return;
  for (auto* row : currentRows())
    row->loadFrom(*patch_);
}

const std::vector<ParamRow*>& EffectEditor::currentRows() const
{
  static const std::vector<ParamRow*> empty;
  if (kind_ == EffectKind::Count)
    return empty;
  return rows_[static_cast<std::size_t>(kind_)];
}

ParamRow* EffectEditor::rowAt(int index) const
{
  const auto& rows = currentRows();
  if (index < 0 || static_cast<std::size_t>(index) >= rows.size())
    return nullptr;
  return rows[static_cast<std::size_t>(index)];
}
