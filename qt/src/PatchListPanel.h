#pragma once

#include <QWidget>

class Patch;
class PatchBank;
class QLineEdit;
class QListWidget;
class QListWidgetItem;

class PatchListPanel : public QWidget
{
  Q_OBJECT

public:
  explicit PatchListPanel(QWidget* parent = nullptr);

  void rebuild(const PatchBank& bank);
  void selectPatch(int index);
  void setFilter(const QString& text);
  void clearFilter();

  [[nodiscard]] int currentPatchIndex() const;
  [[nodiscard]] int visibleCount() const;
  [[nodiscard]] QString rowText(int index) const;
  [[nodiscard]] QSize sizeHint() const override;

signals:
  void patchSelected(int index);

private slots:
  void applyFilter();
  void onCurrentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);

private:
  static QString rowLabel(int index, const Patch& patch);

  QLineEdit* search_ = nullptr;
  QListWidget* list_ = nullptr;
};
