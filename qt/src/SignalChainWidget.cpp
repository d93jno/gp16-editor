#include "SignalChainWidget.h"

#include "Patch.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

constexpr int kChipMinWidth = 96;

const std::array<int, 6> kDefaultBlockAOrder{0, 1, 2, 3, 4, 5};
const std::array<int, 6> kDefaultBlockBOrder{6, 7, 8, 9, 10, 11};

} // namespace

SignalChainWidget::SignalChainWidget(QWidget* parent)
    : QWidget(parent)
{
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(4);

  layout->addWidget(makeRow(QStringLiteral("Block A"), blockAChips_, false));
  layout->addWidget(makeRow(QStringLiteral("Block B"), blockBChips_, true));

  rebuildRow(blockAChips_, false);
  rebuildRow(blockBChips_, true);
}

QToolButton* SignalChainWidget::makeChip(QWidget* parent)
{
  auto* chip = new QToolButton(parent);
  chip->setCheckable(true);
  chip->setMinimumWidth(kChipMinWidth);
  chip->setToolButtonStyle(Qt::ToolButtonTextOnly);
  chip->setStyleSheet(QStringLiteral(
      "QToolButton { padding: 4px 6px; }"
      "QToolButton:!checked { color: #888888; }"
      "QToolButton[selected=\"true\"] { border: 2px solid palette(highlight); }"));
  return chip;
}

QWidget* SignalChainWidget::makeRow(const QString& caption, std::array<QToolButton*, 6>& chips, bool blockB)
{
  auto* row = new QWidget(this);
  auto* rowLayout = new QHBoxLayout(row);
  rowLayout->setContentsMargins(0, 0, 0, 0);
  rowLayout->setSpacing(4);

  auto* label = new QLabel(caption, row);
  label->setMinimumWidth(56);
  rowLayout->addWidget(label);

  for (int i = 0; i < 6; ++i) {
    auto* chip = makeChip(row);
    chips[static_cast<std::size_t>(i)] = chip;
    rowLayout->addWidget(chip);
    connect(chip, &QToolButton::clicked, this, [this, chip]() { onChipClicked(chip); });
  }
  rowLayout->addStretch(1);
  return row;
}

void SignalChainWidget::rebuildRow(std::array<QToolButton*, 6>& chips, bool blockB)
{
  const auto& order = blockB
                           ? (patch_ ? patch_->blockBOrder() : kDefaultBlockBOrder)
                           : (patch_ ? patch_->blockAOrder() : kDefaultBlockAOrder);
  const int mode = patch_ ? patch_->blockB2Mode() : 0;
  const bool distortion = patch_ ? patch_->isDistortion() : true;

  for (int i = 0; i < 6; ++i) {
    auto* chip = chips[static_cast<std::size_t>(i)];
    const int identity = order[static_cast<std::size_t>(i)];
    chip->setProperty("identity", identity);
    chip->setText(QString::fromStdString(Patch::effectName(identity, mode, distortion)));
    chip->setChecked(patch_ && patch_->isEffectEnabled(identity));
    applySelectionStyle(chip, identity == selectedIdentity_);
  }
}

void SignalChainWidget::setPatch(Patch* patch)
{
  patch_ = patch;
  rebuildRow(blockAChips_, false);
  rebuildRow(blockBChips_, true);
}

void SignalChainWidget::onChipClicked(QToolButton* chip)
{
  const int identity = chip->property("identity").toInt();
  const bool wasSelected = identity == selectedIdentity_;

  selectedIdentity_ = identity;
  for (auto* c : blockAChips_)
    applySelectionStyle(c, c == chip);
  for (auto* c : blockBChips_)
    applySelectionStyle(c, c == chip);

  if (!wasSelected) {
    const bool enabled = patch_ && patch_->isEffectEnabled(identity);
    QSignalBlocker blocker(chip);
    chip->setChecked(enabled);
    emit slotSelected(identity);
    return;
  }

  const bool enabled = chip->isChecked();
  if (patch_)
    patch_->setEffectEnabled(identity, enabled);
  emit effectToggled(identity, enabled);
  emit slotSelected(identity);
}

void SignalChainWidget::applySelectionStyle(QToolButton* chip, bool selected)
{
  chip->setProperty("selected", selected);
  chip->style()->unpolish(chip);
  chip->style()->polish(chip);
}

QString SignalChainWidget::chipText(int chainPosition, bool blockB) const
{
  if (chainPosition < 0 || chainPosition >= 6)
    return {};
  const auto& chips = blockB ? blockBChips_ : blockAChips_;
  return chips[static_cast<std::size_t>(chainPosition)]->text();
}

bool SignalChainWidget::chipChecked(int chainPosition, bool blockB) const
{
  if (chainPosition < 0 || chainPosition >= 6)
    return false;
  const auto& chips = blockB ? blockBChips_ : blockAChips_;
  return chips[static_cast<std::size_t>(chainPosition)]->isChecked();
}

int SignalChainWidget::chipIdentity(int chainPosition, bool blockB) const
{
  if (chainPosition < 0 || chainPosition >= 6)
    return -1;
  const auto& chips = blockB ? blockBChips_ : blockAChips_;
  return chips[static_cast<std::size_t>(chainPosition)]->property("identity").toInt();
}
