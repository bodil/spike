#include <QWidget>
#include <QIcon>

class QCommandLineParser;
class QLabel;

class Entry {
public:
  explicit Entry(const QString& name, const QString& exec, const QIcon& icon);

  QString name;
  QString exec;
  QIcon icon;
  QString extra;
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
  QLabel* text;
  QList<Entry> items, activeItems;
  QString current;
  int index;
  const QCommandLineParser& opts;
};
