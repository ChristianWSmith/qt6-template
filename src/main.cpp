#include "appmainwindow/AppMainWindow.h"

#include "logging/logging.h"
#include <QApplication>
#include <QFile>
#include <QGuiApplication>
#include <QIcon>
#include <QLocale>
#include <QTranslator>
#include <cxxopts.hpp>
#include <fmt/core.h>
#include <qdebug.h>

int main(int argc, char *argv[]) {
  cxxopts::Options options(APP_NAME, APP_DESCRIPTION);
  options.add_options()("l, log", "Log level (debug, info, warn, error, none)",
                        cxxopts::value<std::string>()->default_value("info"))(
      "h, help", "Print help");
  cxxopts::ParseResult result = options.parse(argc, argv);

  if (result.count("help")) {
    fprintf(stdout, "%s\n", options.help().c_str());
    return 0;
  }

  setLogLevel(parseLogLevel(result["log"].as<std::string>()));
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
        break;
      } else {
        qWarning() << "Failed to load translation file:" << fileName;
      }
    } else {
      qWarning() << "No translation file:" << fileName;
    }
  }

  AppMainWindow appMainWindow;

  appMainWindow.show();

  return a.exec();
}