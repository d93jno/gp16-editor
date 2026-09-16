#pragma once

#include "PatchChartParser.h"

#include <QDialog>
#include <QString>

class Patch;
class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QSpinBox;

class PatchExportDialog : public QDialog
{
  Q_OBJECT

public:
  PatchExportDialog(const Patch& patch, QWidget* parent = nullptr);

  [[nodiscard]] ChartMetadata metadata() const;
  [[nodiscard]] QString patchName() const;

private:
  QLineEdit* nameEdit_ = nullptr;
  QLineEdit* authorEdit_ = nullptr;
  QPlainTextEdit* commentsEdit_ = nullptr;
  QComboBox* pcGroup_ = nullptr;
  QSpinBox* pcNumber_ = nullptr;
};
