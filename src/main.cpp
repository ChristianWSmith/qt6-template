#include "appmainwindow/AppMainWindow.h"

#include "logging/messageHandler.h"
#include <QApplication>
#include <QFile>
#include <QGuiApplication>
#include <QIcon>
#include <QLocale>
#include <QTranslator>
#include <fmt/core.h>
#include <qdebug.h>

int main(int argc, char *argv[]) {
  qInstallMessageHandler(messageHandler);
  qInfo() << fmt::format("Hello from {} {}!", APP_NAME, APP_VERSION).c_str();

  QApplication a(argc, argv);
  a.setWindowIcon(QIcon(":/icons/app_icon.png"));

  QGuiApplication::setDesktopFileName(APP_ID);

  QTranslator translator;
  const QStringList uiLanguages = QLocale::system().uiLanguages();
  for (const QString &locale : uiLanguages) {
    const QString fileName = ":/i18n/" + QLocale(locale).name() + ".qm";

    if (QFile::exists(fileName)) {
      if (translator.load(fileName)) {
        a.installTranslator(&translator);
        qDebug() << fmt::format("Loaded translation: {}",
                                fileName.toStdString())
                        .c_str();
        break;
      } else {
        qWarning() << fmt::format("Failed to load translation: {}",
                                  fileName.toStdString())
                          .c_str();
      }
    } else {
      qWarning() << fmt::format("No translation file: {}",
                                fileName.toStdString())
                        .c_str();
    }
  }
  qDebug() << "Creating AppMainWindow";
  AppMainWindow appMainWindow;

  qDebug() << "Showing AppMainWindow";
  appMainWindow.show();

  qDebug() << "Starting Application";
  return a.exec();
}