#include <QApplication>
#include <QCommandLineParser>
#include <QDesktopWidget>
#include <QHBoxLayout>
#include <QFile>
#include <QDir>
#include <QSet>
#include <QProcessEnvironment>
#include <QKeyEvent>
#include <QRegExp>
#include <QLabel>
#include <QProcess>

#include <iostream>
#include <algorithm>
#include <signal.h>

#include "x11.h"
#include "spike.h"

QStringList filesInDir(const QDir& dir) {
  return dir.entryList({}, QDir::Files | QDir::Executable, QDir::Name);
}

QStringList filesOnPath(const QStringList& path) {
  QStringList files;
  QStringList::const_iterator i;
  for (i = path.constBegin(); i != path.constEnd(); ++i) {
    files.append(filesInDir(QDir(*i)));
  }
  QSet<QString> fileSet(QSet<QString>::fromList(files));
  QStringList fileList(fileSet.toList());
  fileList.sort(Qt::CaseInsensitive);
  return fileList;
}

QStringList getEnvPath() {
  QString path(QProcessEnvironment::systemEnvironment().value("PATH"));
  return path.split(":");
}

QString readFile(const QString& path) {
  QFile file(path);
  if (!file.exists()) qFatal("ERROR: file not found: %s", path.toUtf8().data());
  if (!file.open(QIODevice::ReadOnly)) {
    qFatal("ERROR: %s", file.errorString().toUtf8().data());
  }
  QByteArray result(file.readAll());
  return QString(result);
}

Selector::Selector(const QCommandLineParser& _opts, QWidget* parent)
  : QWidget(parent,
            Qt::NoDropShadowWindowHint |
            Qt::WindowDoesNotAcceptFocus |
            Qt::FramelessWindowHint)
  , text(new QLabel(this))
  , opts(_opts)
{
  uint margin = opts.value("margin").toUInt();

  QHBoxLayout* l = new QHBoxLayout;
  l->setContentsMargins(margin, margin, margin, margin);
  l->addWidget(text);
  setLayout(l);

  X11::setProp(this, "_NET_WM_DESKTOP", -1);
  X11::setProp(this, "_NET_WM_WINDOW_TYPE", "_NET_WM_WINDOW_TYPE_DOCK");
}

Selector::~Selector() {
  X11::setProp(this, "_NET_WM_STRUT_PARTIAL", {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
}

void Selector::resize(int height, Qt::Edge edge) {
  QDesktopWidget* desktop(QApplication::desktop());
  QRect dg(desktop->screenGeometry());
  QRect g;

  switch (edge) {
  case Qt::LeftEdge:
    g = QRect(0, 0, height, dg.height());
    X11::setProp(this, "_NET_WM_STRUT_PARTIAL",
               {(uint)height, 0, 0, 0, 0, (uint)dg.height(), 0, 0, 0, 0, 0, 0});
    break;
  case Qt::RightEdge:
    g = QRect(dg.width() - height, 0, height, dg.height());
    X11::setProp(this, "_NET_WM_STRUT_PARTIAL",
               {0, (uint)height, 0, 0, 0, 0, 0, (uint)dg.height(), 0, 0, 0, 0});
    break;
  case Qt::TopEdge:
    g = QRect(0, 0, dg.width(), height);
    X11::setProp(this, "_NET_WM_STRUT_PARTIAL",
               {0, 0, (uint)height, 0, 0, 0, 0, 0, 0, (uint)dg.width(), 0, 0});
    break;
  case Qt::BottomEdge:
    g = QRect(0, dg.height() - height, dg.width(), height);
    X11::setProp(this, "_NET_WM_STRUT_PARTIAL",
               {0, 0, 0, (uint)height, 0, 0, 0, 0, 0, 0, 0, (uint)dg.width()});
    break;
  }

  setGeometry(g);
  show();
}

void Selector::select(const QStringList& newItems) {
  items = newItems;
  current = "";
  index = 0;
  updateSelection();
  grabKeyboard();
}

void Selector::endSelection() {
  releaseKeyboard();
  close();
}

void Selector::updateSelection() {
  activeItems = items.filter(QRegExp("^" + QRegExp::escape(current)));

  if (activeItems.size() > 0) {
    while (index >= activeItems.size()) index -= activeItems.size();
    while (index < 0) index += activeItems.size();
  }

  QStringList i(activeItems);
  i[index] = QString("<b style=\"color: %1; background-color: %2\">")
    .arg(opts.value("active")).arg(opts.value("activebg"))
    + i[index] + "</b>";
  text->setText(i.join(" "));
}

void Selector::keyPressEvent(QKeyEvent* event) {
  int key = event->key(), mod = event->modifiers();
  if (key == Qt::Key_Escape ||
      (key == Qt::Key_G && mod == Qt::ControlModifier) ||
      (key == Qt::Key_C && mod == Qt::ControlModifier)) {
    emit cancelled();
    endSelection();
    return;
  } else if (key == Qt::Key_Return) {
    emit selected(activeItems[index]);
    endSelection();
    return;
  } else if (key == Qt::Key_Backspace && mod == Qt::NoModifier) {
    current = current.left(std::max(0, current.size() - 1));
  } else if (key == Qt::Key_Left) {
    index--;
  } else if (key == Qt::Key_Right) {
    index++;
  } else {
    current += event->text();
    index = 0;
  }
  updateSelection();
}

Launcher::Launcher(QObject* parent)
  : QObject(parent)
{}

void Launcher::launch(const QString& program) {
  if (QProcess::startDetached(program))
    emit launched();
  else
    emit failed();
}

static void sigint_handler(int) {
  QApplication::instance()->exit(0);
}

static void mysignal(int sig, void (*handler)(int)) {
  struct sigaction sa;
  sa.sa_handler = handler;
  sa.sa_flags = 0;
  sigaction(sig, &sa, 0);
}

int main(int argc, char* argv[]) {
  QApplication a(argc, argv);
  QApplication::setApplicationName("Spike");
  QApplication::setApplicationVersion("1.0");
  QApplication::setOrganizationName("bodil.org");
  QApplication::setOrganizationDomain("bodil.org");
  mysignal(SIGINT, sigint_handler);

  QCommandLineParser opts;
  opts.setApplicationDescription("An application launcher.");
  opts.addHelpOption();
  opts.addVersionOption();
  QCommandLineOption fontOption({"f", "font"}, "Application font", "font", "sans-serif");
  opts.addOption(fontOption);
  QCommandLineOption bgOption({"b", "background"}, "Background colour", "background",
                              "#171717");
  opts.addOption(bgOption);
  QCommandLineOption fgOption({"t", "text"}, "Text colour", "text",
                              "#F6F3E8");
  opts.addOption(fgOption);
  QCommandLineOption activeOption({"a", "active"}, "Active item colour", "active",
                                  "#EA9847");
  opts.addOption(activeOption);
  QCommandLineOption activeBgOption({"g", "activebg"}, "Active item background colour",
                                    "activebg", "#171717");
  opts.addOption(activeBgOption);
  QCommandLineOption marginOption({"m", "margin"}, "Window margin", "margin", "4");
  opts.addOption(marginOption);
  opts.process(a);

  uint margin = opts.value("margin").toUInt();

  a.setFont(QFont(opts.value("font")));

  QPalette p;
  p.setColor(QPalette::Window, QColor(opts.value("background")));
  p.setColor(QPalette::WindowText, QColor(opts.value("text")));
  a.setPalette(p);

  Selector spike(opts);
  spike.resize(QFontMetrics(a.font()).height() + margin * 2, Qt::BottomEdge);

  Launcher launcher;

  QObject::connect(&spike, &Selector::selected, &launcher, &Launcher::launch);
  spike.select(filesOnPath(getEnvPath()));

  return a.exec();
}
