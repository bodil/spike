#include <QStringList>
#include <QFile>

#include "entry.h"
#include "history.h"

History::History(const QString& path, QObject* parent)
  : QObject(parent)
  , filePath(path)
{}

QStringList readHistory(const QString& path) {
  QFile file(path);
  if (file.exists()) {
    if (!file.open(QIODevice::ReadOnly)) {
      qFatal("ERROR: %s", file.errorString().toUtf8().data());
    }
    QByteArray result(file.readAll());
    return QString::fromUtf8(result).split("\n");
  } else return QStringList();
}

bool writeHistory(const QStringList& entries, const QString& path) {
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly)) {
    qDebug("no open!\n");
    return false;
  }
  QByteArray data(entries.join("\n").toUtf8());
  if (file.write(data) == data.length()) {
    file.close();
    return true;
  } else {
    file.close();
    return false;
  }
}

QStringList History::history() const {
  return readHistory(filePath);
}

QList<Entry> History::sort(const QList<Entry>& inItems) const {
  QStringList history(readHistory(filePath));
  QList<Entry> items(inItems);

  std::sort(items.begin(), items.end(), [&] (const Entry& a, const Entry& b) {
      int ia = history.indexOf(a.exec),
        ib = history.indexOf(b.exec);
      if (ia >= 0 && ib < 0) return true;
      if (ib >= 0 && ia < 0) return false;
      if (ia >= 0 && ib >= 0) return ia > ib;
      return a.name < b.name;
    });
  return items;
}

void History::insert(const QString& entry) {
  QStringList history(readHistory(filePath));

  history.removeAll(entry);
  history.append(entry);

  if (!writeHistory(history, filePath))
    qFatal("ERROR: Unable to write history file.");
}
