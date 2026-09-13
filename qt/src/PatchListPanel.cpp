#include "PatchListPanel.h"

#include "Patch.h"
#include "PatchBank.h"

#include <QAbstractItemView>
#include <QLineEdit>
#include <QListWidget>
#include <QSize>
#include <QVBoxLayout>

PatchListPanel::PatchListPanel(QWidget* parent)
    : QWidget(parent)
{
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(6);

  search_ = new QLineEdit(this);
  search_->setPlaceholderText(tr("Search"));
  search_->setClearButtonEnabled(true);
  layout->addWidget(search_);

  list_ = new QListWidget(this);
  list_->setUniformItemSizes(true);
  list_->setAlternatingRowColors(true);
  list_->setSelectionMode(QAbstractItemView::SingleSelection);
  list_->setSortingEnabled(false);
  list_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  layout->addWidget(list_, 1);

  for (int i = 0; i < PatchBank::kPatchCount; ++i) {
    auto* item = new QListWidgetItem(rowLabel(i, Patch{}), list_);
    item->setData(Qt::UserRole, i);
  }

  connect(search_, &QLineEdit::textChanged, this, &PatchListPanel::applyFilter);
  connect(list_, &QListWidget::currentItemChanged, this, &PatchListPanel::onCurrentItemChanged);
}

QString PatchListPanel::rowLabel(int index, const Patch& patch)
{
  const auto id = QString::fromStdString(Patch::displayIdFor(index));
  if (!patch.isPresent())
    return QStringLiteral("%1  —").arg(id);
  const auto name = QString::fromStdString(patch.name());
  if (name.isEmpty())
    return QStringLiteral("%1  —").arg(id);
  return QStringLiteral("%1  %2").arg(id, name);
}

void PatchListPanel::rebuild(const PatchBank& bank)
{
  const int previous = currentPatchIndex();
  const int n = list_->count();
  for (int i = 0; i < n && i < PatchBank::kPatchCount; ++i) {
    auto* item = list_->item(i);
    if (!item)
      continue;
    const int index = item->data(Qt::UserRole).toInt();
    item->setText(rowLabel(index, bank.patchAt(index)));
  }
  applyFilter();
  if (previous >= 0)
    selectPatch(previous);
}

void PatchListPanel::selectPatch(int index)
{
  if (index < 0 || index >= list_->count()) {
    list_->setCurrentItem(nullptr);
    return;
  }
  auto* item = list_->item(index);
  if (!item)
    return;
  list_->setCurrentItem(item);
  list_->scrollToItem(item, QAbstractItemView::PositionAtCenter);
}

void PatchListPanel::setFilter(const QString& text)
{
  search_->setText(text);
}

void PatchListPanel::clearFilter()
{
  search_->clear();
}

QSize PatchListPanel::sizeHint() const
{
  return {240, 400};
}

int PatchListPanel::currentPatchIndex() const
{
  const auto* item = list_->currentItem();
  if (!item)
    return -1;
  return item->data(Qt::UserRole).toInt();
}

int PatchListPanel::visibleCount() const
{
  int n = 0;
  for (int i = 0; i < list_->count(); ++i) {
    const auto* item = list_->item(i);
    if (item && !item->isHidden())
      ++n;
  }
  return n;
}

QString PatchListPanel::rowText(int index) const
{
  if (index < 0 || index >= list_->count())
    return {};
  const auto* item = list_->item(index);
  return item ? item->text() : QString();
}

void PatchListPanel::applyFilter()
{
  const auto needle = search_->text().trimmed();
  for (int i = 0; i < list_->count(); ++i) {
    auto* item = list_->item(i);
    if (!item)
      continue;
    const bool match =
        needle.isEmpty() || item->text().contains(needle, Qt::CaseInsensitive);
    item->setHidden(!match);
  }
}

void PatchListPanel::onCurrentItemChanged(QListWidgetItem* current, QListWidgetItem* previous)
{
  Q_UNUSED(previous);
  if (!current) {
    emit patchSelected(-1);
    return;
  }
  emit patchSelected(current->data(Qt::UserRole).toInt());
}
