#include <QWidget>
#include <QIcon>
#include <QAbstractListModel>

class QCommandLineParser;
class QListView;

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

class Launcher : public QObject {
  Q_OBJECT

public:
  explicit Launcher(QObject* = 0);

public slots:
  void launch(const Entry&);

signals:
  void launched();
  void failed();
};

class Selector : public QWidget {
  Q_OBJECT

public:
  explicit Selector(const QCommandLineParser&, QWidget* = 0);
  ~Selector();

  void resize(int, Qt::Edge);

public slots:
  void select(const QList<Entry>&);
  void endSelection();
  void updateSelection();

signals:
  void selected(const Entry&);
  void cancelled();

protected:
  void keyPressEvent(QKeyEvent*);

private:
  QListView* list;
  EntryModel* model;
  QList<Entry> items, activeItems;
  QString current;
  int index;
  const QCommandLineParser& opts;
};
