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
#include <QListView>
#include <QProcess>
#include <QIcon>

#include <iostream>
#include <algorithm>
#include <signal.h>

#include "x11.h"
#include "spike.h"

// -- Utilities

QStringList filesInDir(const QDir& dir) {
  return dir.entryList({}, QDir::Files | QDir::Executable, QDir::Name);
}

QList<Entry> filesOnPath(const QStringList& path) {
  QStringList files;
  QStringList::const_iterator i;
  for (i = path.constBegin(); i != path.constEnd(); ++i) {
    files.append(filesInDir(QDir(*i)));
  }
  QSet<QString> fileSet(QSet<QString>::fromList(files));
  files = fileSet.toList();
  files.sort(Qt::CaseInsensitive);
  QList<Entry> entries;
  for (i = files.constBegin(); i != files.constEnd(); ++i) {
    entries.append(Entry(*i, *i, QIcon()));
  }
  return entries;
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

// -- XDG

QStringList getXdgApplicationPaths() {
  QString env(QProcessEnvironment::systemEnvironment().value("XDG_DATA_DIRS"));
  return env.split(":").replaceInStrings(QRegExp("$"), "/applications");
}

QString matchKey(const QString& key, const QString& s) {
  QRegExp m(QString("^%1\\s*=\\s*(.*)$").arg(key));
  return m.exactMatch(s) ? m.capturedTexts()[1] : QString();
}

Entry parseApplication(const QString& data) {
  QString name, exec, icon, type, read;
  QStringList lines = data.split("\n");
  QStringList::const_iterator i;
  for (i = lines.constBegin(); i != lines.constEnd(); ++i) {
    read = matchKey("Type", *i);
    if (!read.isNull()) type = read;
    read = matchKey("Name", *i);
    if (!read.isNull()) name = read;
    read = matchKey("Exec", *i);
    if (!read.isNull()) exec = read;
    read = matchKey("Icon", *i);
    if (!read.isNull()) icon = read;
  }
  if (type.toLower() != "application") {
    return Entry("", "", QIcon());
  } else {
    return Entry(name, exec, icon.isEmpty() ? QIcon() : QIcon(icon));
  }
}

QStringList applicationsInDir(const QDir& dir) {
  return dir.entryList({"*.desktop"}, QDir::Files | QDir::Readable, QDir::Name);
}

bool entryLessThan(Entry a, Entry b) {
  return a.name < b.name;
}

QList<Entry> applicationsOnPath(const QStringList& path) {
  QStringList files;
  QStringList::const_iterator i, j;
  for (i = path.constBegin(); i != path.constEnd(); ++i) {
    QDir dir(*i);
    QStringList appsInDir = applicationsInDir(dir);
    for (j = appsInDir.constBegin(); j != appsInDir.constEnd(); ++j) {
      files += dir.filePath(*j);
    }
  }
  QSet<QString> fileSet(QSet<QString>::fromList(files));
  files = fileSet.toList();
  QList<Entry> entries;
  for (i = files.constBegin(); i != files.constEnd(); ++i) {
    Entry e = parseApplication(readFile(*i));
    if (!e.name.isEmpty()) entries += e;
  }
  std::sort(entries.begin(), entries.end(), entryLessThan);
  return entries;
}

// -- Entry

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

// -- Selector

Selector::Selector(const QCommandLineParser& _opts, QWidget* parent)
  : QWidget(parent,
            Qt::NoDropShadowWindowHint |
            Qt::WindowDoesNotAcceptFocus |
            Qt::FramelessWindowHint)
  , list(new QListView(this))
  , opts(_opts)
{
  uint margin = opts.value("margin").toUInt();
  uint fontSize = QFontMetrics(QApplication::font()).height();

  model = new EntryModel({}, QColor(opts.value("error")), this);

  list->setFlow(QListView::LeftToRight);
  list->setSelectionMode(QAbstractItemView::SingleSelection);
  list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  list->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  list->setSpacing(margin);
  list->setIconSize(QSize(fontSize, fontSize));
  list->setFrameStyle(QFrame::NoFrame);
  list->setMaximumHeight(QFontMetrics(QApplication::font()).height() + (margin * 2));

  QHBoxLayout* l = new QHBoxLayout;
  l->setContentsMargins(0, 0, 0, 0);
  l->addWidget(list);
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

void Selector::select(const QList<Entry>& newItems) {
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

QList<Entry> filterEntries(const QList<Entry>& entries, const QRegExp& re) {
  QList<Entry> filtered;
  QList<Entry>::const_iterator i;
  for (i = entries.constBegin(); i != entries.constEnd(); ++i) {
    if (re.exactMatch((*i).name) ||
        (!(*i).extra.isEmpty() && re.exactMatch((*i).extra))) filtered += *i;
  }
  return filtered;
}

QStringList entryNames(const QList<Entry>& entries) {
  QStringList names;
  QList<Entry>::const_iterator i;
  for (i = entries.constBegin(); i != entries.constEnd(); ++i) {
    names += (*i).name;
  }
  return names;
}

void Selector::updateSelection() {
  activeItems = filterEntries(items, QRegExp("^" + QRegExp::escape(current) + ".*",
                                             Qt::CaseInsensitive));

  if (activeItems.size() > 0) {
    while (index >= activeItems.size()) index -= activeItems.size();
    while (index < 0) index += activeItems.size();
  } else {
    delete model;
    model = new EntryModel({Entry("", "", QIcon())}, QColor(opts.value("error")), this);
    list->setModel(model);
    return;
  }

  delete model;
  model = new EntryModel(activeItems, QColor(opts.value("error")), this);
  list->setModel(model);
  list->selectionModel()->setCurrentIndex(model->index(index, 0),
                                          QItemSelectionModel::SelectCurrent);
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
    if (activeItems.size() > 0) {
      emit selected(activeItems[index]);
      endSelection();
    }
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

// -- Launcher

Launcher::Launcher(QObject* parent)
  : QObject(parent)
{}

void Launcher::launch(const Entry& program) {
  std::cout << "Launching '" << program.exec.toLocal8Bit().data() << "'...";
  if (QProcess::startDetached(program.exec))
    emit launched();
  else
    emit failed();
}

// -- Main

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
  QCommandLineOption sourceOption({"s", "source"}, "Application source [path, xdg]", "source", "xdg");
  opts.addOption(sourceOption);
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
  QCommandLineOption errorOption({"e", "error"}, "Error message colour",
                                    "error", "#E2434C");
  opts.addOption(errorOption);
  QCommandLineOption marginOption({"m", "margin"}, "Window margin", "margin", "4");
  opts.addOption(marginOption);
  opts.process(a);

  uint margin = opts.value("margin").toUInt();

  a.setFont(QFont(opts.value("font")));

  QPalette p;
  p.setColor(QPalette::Base, QColor(opts.value("background")));
  p.setColor(QPalette::Text, QColor(opts.value("text")));
  p.setColor(QPalette::Highlight, QColor(opts.value("activebg")));
  p.setColor(QPalette::HighlightedText, QColor(opts.value("active")));
  p.setColor(QPalette::Window, QColor(opts.value("background")));
  a.setPalette(p);

  Selector spike(opts);
  spike.resize(QFontMetrics(a.font()).height() + margin * 2, Qt::BottomEdge);

  Launcher launcher;
  QObject::connect(&spike, &Selector::selected, &launcher, &Launcher::launch);

  if (opts.value("source") == "path") {
    spike.select(filesOnPath(getEnvPath()));
  } else if (opts.value("source") == "xdg") {
    spike.select(applicationsOnPath(getXdgApplicationPaths()));
  } else {
    std::cout << "Unknown application source '"
              << opts.value("source").toLocal8Bit().data()
              << "'.\n";
    exit(1);
  }

  return a.exec();
}
