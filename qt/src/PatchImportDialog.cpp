#include "PatchImportDialog.h"

#include "EffectSpecs.h"
#include "Patch.h"
#include "PatchBank.h"
#include "PatchListPanel.h"

#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSplitter>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string_view>

namespace {

const char* slotTag(int identity)
{
  static constexpr const char* kTags[] = {"A-1", "A-2", "A-3", "A-4", "A-5", "A-6",
                                          "B-1", "B-2", "B-3", "B-4", "B-5", "B-6"};
  if (identity < 0 || identity >= Patch::kEffectCount)
    return "?";
  return kTags[identity];
}

std::string formatHz(double hz)
{
  std::ostringstream os;
  os << std::fixed;
  if (hz >= 1000.0)
    os << std::setprecision(2) << (hz / 1000.0) << " kHz";
  else
    os << std::setprecision(0) << hz << " Hz";
  return os.str();
}

QString formatParamValue(const ParamSpec& spec, int raw)
{
  if (spec.type == ParamType::Combo && spec.comboItems && spec.comboCount > 0) {
    const int i = std::clamp(raw, spec.min, spec.max);
    if (i >= 0 && i < spec.comboCount)
      return QString::fromUtf8(spec.comboItems[i]);
  }
  if (spec.type == ParamType::Checkbox)
    return raw ? QStringLiteral("ON") : QStringLiteral("OFF");
  if ((spec.offset == 0x4F || spec.offset == 0x53) && raw >= spec.max)
    return QStringLiteral("THRU");
  if (spec.offset == 0x51) {
    const double sec = raw <= 45 ? 0.5 + static_cast<double>(raw) * 0.1
                                 : 5.5 + static_cast<double>(raw - 46) * 0.5;
    return QString::number(sec, 'f', 1) + QStringLiteral(" sec");
  }
  if (spec.offset == 0x4F || spec.offset == 0x53) {
    ParamSpec freq = spec;
    freq.min = 0;
    freq.max = 199;
    freq.transform = DisplayTransform::FreqLog;
    freq.displayMin = 500;
    freq.displayMax = 8000;
    return QString::fromStdString(formatHz(rawToDisplay(freq, raw)));
  }

  const double display = rawToDisplay(spec, raw);
  switch (spec.transform) {
    case DisplayTransform::LevelDb:
      if (display == 0.0)
        return QStringLiteral("0.0 dB");
      return QString::asprintf("%+.1f dB", display);
    case DisplayTransform::QValue:
      return QString::number(display, 'f', 1);
    case DisplayTransform::FreqLinear:
    case DisplayTransform::FreqLog:
      return QString::fromStdString(formatHz(display));
    default: {
      const int n = static_cast<int>(std::lround(display));
      if (spec.suffix && std::string_view(spec.suffix).find("ms") != std::string_view::npos)
        return QString::number(n) + QStringLiteral(" msec");
      return QString::number(n);
    }
  }
}

QTreeWidgetItem* addEffectItem(QTreeWidget* tree, const QString& title)
{
  auto* item = new QTreeWidgetItem(tree);
  item->setText(0, title);
  return item;
}

} // namespace

PatchImportDialog::PatchImportDialog(const ParsedChart& chart, const PatchBank& bank,
                                     int defaultIndex, QWidget* parent)
    : QDialog(parent)
    , destinationIndex_(defaultIndex >= 0 && defaultIndex < PatchBank::kPatchCount ? defaultIndex
                                                                                   : 0)
{
  setWindowTitle(QStringLiteral("Import Patch"));
  setObjectName(QStringLiteral("patchImportDialog"));
  resize(860, 560);

  auto* root = new QVBoxLayout(this);
  auto* splitter = new QSplitter(Qt::Horizontal, this);
  splitter->setChildrenCollapsible(false);

  auto* preview = new QWidget(splitter);
  auto* previewLayout = new QVBoxLayout(preview);
  previewLayout->setContentsMargins(0, 0, 0, 0);

  nameLabel_ = new QLabel(preview);
  nameLabel_->setObjectName(QStringLiteral("importName"));
  auto nameFont = nameLabel_->font();
  nameFont.setBold(true);
  nameFont.setPointSizeF(nameFont.pointSizeF() + 2);
  nameLabel_->setFont(nameFont);
  nameLabel_->setText(QString::fromStdString(chart.name.empty() ? "(unnamed)" : chart.name));
  previewLayout->addWidget(nameLabel_);

  authorLabel_ = new QLabel(preview);
  authorLabel_->setObjectName(QStringLiteral("importAuthor"));
  authorLabel_->setWordWrap(true);
  authorLabel_->setText(chart.author.empty()
                            ? QStringLiteral("Author: —")
                            : QStringLiteral("Author: %1").arg(QString::fromStdString(chart.author)));
  previewLayout->addWidget(authorLabel_);

  commentsLabel_ = new QLabel(preview);
  commentsLabel_->setObjectName(QStringLiteral("importComments"));
  commentsLabel_->setWordWrap(true);
  commentsLabel_->setText(
      chart.comments.empty()
          ? QStringLiteral("Comments: —")
          : QStringLiteral("Comments: %1").arg(QString::fromStdString(chart.comments)));
  previewLayout->addWidget(commentsLabel_);

  if (chart.programChangeGroup && chart.programChangeNumber) {
    auto* pc = new QLabel(preview);
    pc->setObjectName(QStringLiteral("importProgramChange"));
    pc->setText(QStringLiteral("Program change: %1%2")
                    .arg(*chart.programChangeGroup)
                    .arg(*chart.programChangeNumber));
    previewLayout->addWidget(pc);
  }

  auto* effectsBox = new QGroupBox(QStringLiteral("Effects"), preview);
  auto* effectsLayout = new QVBoxLayout(effectsBox);
  effects_ = new QTreeWidget(effectsBox);
  effects_->setObjectName(QStringLiteral("importEffects"));
  effects_->setHeaderHidden(true);
  effects_->setRootIsDecorated(true);
  effects_->setColumnCount(1);
  effectsLayout->addWidget(effects_);
  previewLayout->addWidget(effectsBox, 1);
  fillEffects(chart);

  auto* warnBox = new QGroupBox(QStringLiteral("Warnings"), preview);
  auto* warnLayout = new QVBoxLayout(warnBox);
  warnings_ = new QListWidget(warnBox);
  warnings_->setObjectName(QStringLiteral("importWarnings"));
  warnLayout->addWidget(warnings_);
  previewLayout->addWidget(warnBox);
  fillWarnings(chart);

  auto* destPane = new QWidget(splitter);
  auto* destLayout = new QVBoxLayout(destPane);
  destLayout->setContentsMargins(0, 0, 0, 0);
  destinationLabel_ = new QLabel(destPane);
  destinationLabel_->setObjectName(QStringLiteral("importDestination"));
  destLayout->addWidget(destinationLabel_);
  destList_ = new PatchListPanel(destPane);
  destList_->setObjectName(QStringLiteral("importDestList"));
  destList_->rebuild(bank);
  destLayout->addWidget(destList_, 1);

  splitter->addWidget(preview);
  splitter->addWidget(destPane);
  splitter->setStretchFactor(0, 3);
  splitter->setStretchFactor(1, 2);
  splitter->setSizes({520, 320});
  root->addWidget(splitter, 1);

  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  auto* importBtn = buttons->button(QDialogButtonBox::Ok);
  importBtn->setText(QStringLiteral("Import"));
  importBtn->setObjectName(QStringLiteral("importButton"));
  buttons->button(QDialogButtonBox::Cancel)->setObjectName(QStringLiteral("cancelButton"));
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  root->addWidget(buttons);

  connect(destList_, &PatchListPanel::patchSelected, this, [this](int index) {
    if (index < 0)
      return;
    destinationIndex_ = index;
    updateDestinationLabel();
  });
  destList_->selectPatch(destinationIndex_);
  updateDestinationLabel();
}

void PatchImportDialog::setDestinationIndex(int index)
{
  if (index < 0 || index >= PatchBank::kPatchCount)
    return;
  destinationIndex_ = index;
  destList_->selectPatch(index);
  updateDestinationLabel();
}

QString PatchImportDialog::patchName() const
{
  return nameLabel_ ? nameLabel_->text() : QString();
}

QString PatchImportDialog::authorText() const
{
  return authorLabel_ ? authorLabel_->text() : QString();
}

QString PatchImportDialog::commentsText() const
{
  return commentsLabel_ ? commentsLabel_->text() : QString();
}

int PatchImportDialog::warningCount() const
{
  return warnings_ ? warnings_->count() : 0;
}

QString PatchImportDialog::warningAt(int index) const
{
  if (!warnings_ || index < 0 || index >= warnings_->count())
    return {};
  const auto* item = warnings_->item(index);
  return item ? item->text() : QString();
}

QString PatchImportDialog::effectsSummary() const
{
  if (!effects_)
    return {};
  QStringList lines;
  for (int i = 0; i < effects_->topLevelItemCount(); ++i) {
    const auto* item = effects_->topLevelItem(i);
    if (!item)
      continue;
    lines << item->text(0);
    for (int c = 0; c < item->childCount(); ++c) {
      const auto* child = item->child(c);
      if (child)
        lines << child->text(0);
    }
  }
  return lines.join(QLatin1Char('\n'));
}

void PatchImportDialog::fillEffects(const ParsedChart& chart)
{
  const Patch patch = chartToPatch(chart);
  for (int id = 0; id < Patch::kEffectCount; ++id) {
    const bool on = patch.isEffectEnabled(id);
    const EffectKind kind = kindForSlot(id, patch.blockB2Mode(), patch.isDistortion());
    const auto& spec = specFor(kind);
    const auto title = QStringLiteral("%1  %2  %3")
                           .arg(QString::fromUtf8(slotTag(id)), QString::fromUtf8(spec.name),
                                on ? QStringLiteral("ON") : QStringLiteral("OFF"));
    auto* item = addEffectItem(effects_, title);
    if (!on)
      continue;
    for (const auto& param : spec.params) {
      auto* child = new QTreeWidgetItem(item);
      QString label = QString::fromUtf8(param.label);
      if (param.group)
        label = QStringLiteral("%1 %2").arg(QString::fromUtf8(param.group), label);
      child->setText(0, QStringLiteral("%1  %2").arg(label, formatParamValue(param, readParam(patch, param))));
    }
    item->setExpanded(true);
  }

  auto* globalsItem = addEffectItem(effects_, QStringLiteral("Global"));
  for (const auto& param : allGlobalParams()) {
    auto* child = new QTreeWidgetItem(globalsItem);
    child->setText(0, QStringLiteral("%1  %2")
                          .arg(QString::fromUtf8(param.label),
                               formatParamValue(param, readParam(patch, param))));
  }
  globalsItem->setExpanded(true);
}

void PatchImportDialog::fillWarnings(const ParsedChart& chart)
{
  for (const auto& w : chart.warnings)
    warnings_->addItem(QString::fromStdString(w));
}

void PatchImportDialog::updateDestinationLabel()
{
  if (!destinationLabel_)
    return;
  destinationLabel_->setText(
      QStringLiteral("Destination: %1")
          .arg(QString::fromStdString(Patch::displayIdFor(destinationIndex_))));
}
