#pragma once

#include <QWidget>

#include <array>

class Patch;
class QToolButton;

class SignalChainWidget : public QWidget
{
  Q_OBJECT

public:
  explicit SignalChainWidget(QWidget* parent = nullptr);

  // Points at a Patch owned by the caller (typically a PatchBank slot).
  // Pass nullptr to show the default, empty chain.
  void setPatch(Patch* patch);

  [[nodiscard]] int selectedIdentity() const { return selectedIdentity_; }

  [[nodiscard]] QString chipText(int chainPosition, bool blockB) const;
  [[nodiscard]] bool chipChecked(int chainPosition, bool blockB) const;
  [[nodiscard]] int chipIdentity(int chainPosition, bool blockB) const;

signals:
  void slotSelected(int identity);
  void effectToggled(int identity, bool enabled);

private:
  QToolButton* makeChip(QWidget* parent);
  QWidget* makeRow(const QString& caption, std::array<QToolButton*, 6>& chips, bool blockB);
  void rebuildRow(std::array<QToolButton*, 6>& chips, bool blockB);
  void onChipClicked(QToolButton* chip);
  static void applySelectionStyle(QToolButton* chip, bool selected);

  Patch* patch_ = nullptr;
  std::array<QToolButton*, 6> blockAChips_{};
  std::array<QToolButton*, 6> blockBChips_{};
  int selectedIdentity_ = -1;
};
