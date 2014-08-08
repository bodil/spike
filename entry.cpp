#include <QBrush>

#include "entry.h"

Entry::Entry(const QString& name, const QString& exec, const QIcon& icon)
  : name(name)
  , exec(exec)
  , icon(icon)
{
  extra = exec.split("/").last();
}

EntryModel::EntryModel(const QList<Entry>& newEntries, const QColor& errorColor, QObject* parent)
  : QAbstractListModel(parent)
  , entries(newEntries)
  , error(errorColor)
{}

int EntryModel::rowCount(const QModelIndex&) const {
  return entries.size();
}

QVariant EntryModel::data(const QModelIndex& index, int role) const {
  Entry e = entries[index.row()];
  if (e.name.isEmpty() && e.exec.isEmpty()) {
    // Error message
    if (role == Qt::DisplayRole)
      return "No match";
    if (role == Qt::DecorationRole)
      return QIcon(":/error.png");
    if (role == Qt::ForegroundRole)
      return QBrush(error);
    return QVariant();
  } else {
    if (role == Qt::DisplayRole)
      return e.name;
    if (role == Qt::DecorationRole)
      return e.icon;
    return QVariant();
  }
}
