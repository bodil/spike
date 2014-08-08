#include <QWidget>

class QCommandLineParser;
class QLabel;

class Launcher : public QObject {
  Q_OBJECT

public:
  explicit Launcher(QObject* = 0);

public slots:
  void launch(const QString&);

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
  void select(const QStringList&);
  void endSelection();
  void updateSelection();

signals:
  void selected(const QString&);
  void cancelled();

protected:
  void keyPressEvent(QKeyEvent*);

private:
  QLabel* text;
  QStringList items, activeItems;
  QString current;
  int index;
  const QCommandLineParser& opts;
};
