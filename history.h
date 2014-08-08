#include <QObject>

class Entry;

class History : public QObject {
  Q_OBJECT

public:
  explicit History(const QString&, QObject* = 0);

  QStringList history() const;
  QList<Entry> sort(const QList<Entry>&) const;

public slots:
  void insert(const QString&);

private:
  QString filePath;
};
