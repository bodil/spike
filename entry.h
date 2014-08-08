#include <QAbstractListModel>
#include <QIcon>

class Entry {
public:
  explicit Entry(const QString& name, const QString& exec, const QIcon& icon);

  QString name;
  QString exec;
  QIcon icon;
  QString extra;
};

class EntryModel : public QAbstractListModel {
  Q_OBJECT

public:
  explicit EntryModel(const QList<Entry>&, const QColor&, QObject* parent = 0);

  int rowCount(const QModelIndex& = QModelIndex()) const;
  QVariant data(const QModelIndex& = QModelIndex(), int = Qt::DisplayRole) const;

private:
  QList<Entry> entries;
  QColor error;
};
