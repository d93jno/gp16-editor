#include "PatchExportDialog.h"

#include "Patch.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

PatchExportDialog::PatchExportDialog(const Patch& patch, QWidget* parent)
    : QDialog(parent)
{
  setWindowTitle(QStringLiteral("Export Patch"));
  setObjectName(QStringLiteral("patchExportDialog"));
  resize(480, 360);

  auto* root = new QVBoxLayout(this);
  auto* form = new QFormLayout();

  nameEdit_ = new QLineEdit(QString::fromStdString(patch.name()), this);
  nameEdit_->setObjectName(QStringLiteral("exportName"));
  nameEdit_->setReadOnly(true);
  form->addRow(QStringLiteral("Patch name"), nameEdit_);

  authorEdit_ = new QLineEdit(this);
  authorEdit_->setObjectName(QStringLiteral("exportAuthor"));
  form->addRow(QStringLiteral("Author"), authorEdit_);

  commentsEdit_ = new QPlainTextEdit(this);
  commentsEdit_->setObjectName(QStringLiteral("exportComments"));
  commentsEdit_->setPlaceholderText(QStringLiteral("Optional. Not stored on the GP-16."));
  form->addRow(QStringLiteral("Comments"), commentsEdit_);

  auto* pcRow = new QWidget(this);
  auto* pcLayout = new QHBoxLayout(pcRow);
  pcLayout->setContentsMargins(0, 0, 0, 0);
  pcGroup_ = new QComboBox(pcRow);
  pcGroup_->setObjectName(QStringLiteral("exportPcGroup"));
  pcGroup_->addItem(QStringLiteral("(none)"), QString());
  pcGroup_->addItem(QStringLiteral("A"), QStringLiteral("A"));
  pcGroup_->addItem(QStringLiteral("B"), QStringLiteral("B"));
  pcNumber_ = new QSpinBox(pcRow);
  pcNumber_->setObjectName(QStringLiteral("exportPcNumber"));
  pcNumber_->setRange(0, 128);
  pcNumber_->setSpecialValueText(QStringLiteral("—"));
  pcNumber_->setValue(0);
  pcLayout->addWidget(pcGroup_);
  pcLayout->addWidget(pcNumber_, 1);
  form->addRow(QStringLiteral("Program change"), pcRow);

  root->addLayout(form);

  auto* note = new QLabel(
      QStringLiteral("Author, comments, and program change are documentation only — "
                     "the GP-16 patch buffer has no fields for them."),
      this);
  note->setWordWrap(true);
  root->addWidget(note);

  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
  buttons->button(QDialogButtonBox::Save)->setObjectName(QStringLiteral("exportSaveButton"));
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  root->addWidget(buttons);
}

ChartMetadata PatchExportDialog::metadata() const
{
  ChartMetadata meta;
  if (authorEdit_)
    meta.author = authorEdit_->text().trimmed().toStdString();
  if (commentsEdit_)
    meta.comments = commentsEdit_->toPlainText().trimmed().toStdString();
  if (pcGroup_ && pcNumber_ && pcNumber_->value() > 0) {
    const auto g = pcGroup_->currentData().toString();
    if (g.size() == 1) {
      meta.programChangeGroup = g.at(0).toLatin1();
      meta.programChangeNumber = pcNumber_->value();
    }
  }
  return meta;
}

QString PatchExportDialog::patchName() const
{
  return nameEdit_ ? nameEdit_->text() : QString();
}
