#pragma once

#include "PatchChartParser.h"

#include <QDialog>
#include <QString>

class PatchBank;
class PatchListPanel;
class QLabel;
class QListWidget;
class QTreeWidget;

class PatchImportDialog : public QDialog
{
  Q_OBJECT

public:
  PatchImportDialog(const ParsedChart& chart, const PatchBank& bank, int defaultIndex,
                    QWidget* parent = nullptr);

  [[nodiscard]] int destinationIndex() const { return destinationIndex_; }
  void setDestinationIndex(int index);

  [[nodiscard]] QString patchName() const;
  [[nodiscard]] QString authorText() const;
  [[nodiscard]] QString commentsText() const;
  [[nodiscard]] int warningCount() const;
  [[nodiscard]] QString warningAt(int index) const;
  [[nodiscard]] QString effectsSummary() const;

private:
  void fillEffects(const ParsedChart& chart);
  void fillWarnings(const ParsedChart& chart);
  void updateDestinationLabel();

  int destinationIndex_ = 0;
  QLabel* nameLabel_ = nullptr;
  QLabel* authorLabel_ = nullptr;
  QLabel* commentsLabel_ = nullptr;
  QLabel* destinationLabel_ = nullptr;
  QTreeWidget* effects_ = nullptr;
  QListWidget* warnings_ = nullptr;
  PatchListPanel* destList_ = nullptr;
};
