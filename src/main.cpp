#include "app_main_window/AppMainWindow.h"

#include <QApplication>
#include <QFile>
#include <QGuiApplication>
#include <QIcon>
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

    if (QFile::exists(":/i18n/" + baseName + ".qm")) {
      if (translator.load(":/i18n/" + baseName)) {
        a.installTranslator(&translator);
        break;
      }
    }
  }

  AppMainWindow appMainWindow;

  appMainWindow.show();

  return a.exec();
}