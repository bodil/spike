#include <QWidget>

class Entry;
class EntryModel;
class QCommandLineParser;
class QListView;

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
