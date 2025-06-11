#include "app.h"
#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <fmt/core.h>

int main(int argc, char *argv[]) {
  fmt::print("Hello from {} {}!\n", APP_NAME, APP_VERSION);
  QApplication a(argc, argv);
  a.setWindowIcon(QIcon(":/icons/app_icon.png"));
  QGuiApplication::setDesktopFileName(APP_ID);

  QTranslator translator;
  const QStringList uiLanguages = QLocale::system().uiLanguages();
  for (const QString &locale : uiLanguages) {
    const QString baseName = QLocale(locale).name();
    if (translator.load(":/i18n/" + baseName)) {
      a.installTranslator(&translator);
      break;
    }
  }
  MainWindow w;
  w.show();
  return a.exec();
}
