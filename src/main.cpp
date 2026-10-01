#include "appmainwindow/AppMainWindow.h"

#include "logging/logging.h"
#include "platform/theme/theme.hpp"
#include "services/registry/ServiceRegistry.hpp"
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QIcon>
#include <QLocale>
#include <QLoggingCategory>
#include <QTranslator>
#include <cxxopts.hpp>
#include <fmt/format.h>
#include <iostream>

void setupLocalization(QTranslator &translator) {
  QLocale locale = QLocale::system();
  QString langCode = locale.name();
  QString baseLang = langCode.section('_', 0, 0);

  if (translator.load(QString("%1_%2.qm").arg(APP_NAME).arg(baseLang),
                      ":/i18n")) {
    QCoreApplication::installTranslator(&translator);
  } else {
    qCWarning(appMain) << "Failed to load translation for" << baseLang;
  }
}

int main(int argc, char *argv[]) {
  try {
    // CLI parsing and configureLogLevel (logging module) live behind the
    // same try/catch so option errors are classified separately from
    // post-register bootstrap failures.
    cxxopts::Options options(APP_NAME, APP_DESCRIPTION);
    options.add_options()(
        "l,log", "Log level (debug, info, warn, error, none)",
        cxxopts::value<std::string>()->default_value("info"))(
        "smoke-test",
        "Run in smoke test mode (exits immediately after setup)")(
        "h,help", "Print help");

    cxxopts::ParseResult parsedArgs = options.parse(argc, argv);
    if (parsedArgs.contains("help")) {
      std::cout << options.help().c_str() << '\n';
      return 0;
    }

    // F-11: custom handler retained (pedagogy + fatal/sink behavior);
    // see logging.cpp. qSetMessagePattern is the formatting-only alternative.
    qInstallMessageHandler(messageHandler);
    configureLogLevel(parsedArgs.operator[]("log").as<std::string>());

    // fmt demo (Keep list): Hello message uses fmt; logging.cpp uses fmt too.
    qCInfo(appMain) << fmt::format("Hello from {} {}!", APP_NAME, APP_VERSION)
                           .c_str();

    QApplication app(argc, argv);

    services::registerAll();
    QApplication::setApplicationName(QString::fromStdString(APP_NAME));
    QApplication::setOrganizationName(
        QString::fromStdString(ORGANIZATION_NAME));
    QApplication::setWindowIcon(QIcon(":/icons/app_icon.png"));
    QGuiApplication::setDesktopFileName(APP_ID);

    // Declared after QApplication so it is destroyed before the application
    // on scope exit. Removed explicitly before return so QCoreApplication
    // never observes a destroyed translator.
    QTranslator translator;
    setupLocalization(translator);

    AppMainWindow mainWindow;

    setTheme();

    if (parsedArgs.contains("smoke-test")) {
      qCInfo(appMain)
          << "Smoke test successful: Application initialized and exiting.";
      services::unregisterAll();
      QCoreApplication::removeTranslator(&translator);
      return 0;
    }

    mainWindow.show();

    const int exitCode = QApplication::exec();
    services::unregisterAll();
    QCoreApplication::removeTranslator(&translator);
    return exitCode;
  } catch (const cxxopts::exceptions::exception &e) {
    std::cerr << "Invalid command line: " << e.what() << '\n';
    return 2;
  } catch (const std::exception &e) {
    // Lifecycle contract: every path after registerAll() must unregister.
    // (CLI parse errors are handled above and never reach registerAll.)
    services::unregisterAll();
    std::cerr << "UNCAUGHT EXCEPTION: " << e.what() << '\n';
    return 1;
  } catch (...) {
    services::unregisterAll();
    std::cerr << "UNKNOWN EXCEPTION\n";
    return 1;
  }
}
