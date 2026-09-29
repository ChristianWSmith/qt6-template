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
#include <QTranslator>
#include <cxxopts.hpp>
#include <fmt/format.h>
#include <iostream>

// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
std::optional<cxxopts::ParseResult>
parseCommandLine(int argc, char *argv[]) {
  cxxopts::Options options(APP_NAME, APP_DESCRIPTION);
  options.add_options()("l,log", "Log level (debug, info, warn, error, none)",
                        cxxopts::value<std::string>()->default_value("info"))(
      "smoke-test", "Run in smoke test mode (exits immediately after setup)")(
      "h,help", "Print help");

  cxxopts::ParseResult parsedArgs = options.parse(argc, argv);

  if (parsedArgs.contains("help")) {
    std::cout << options.help().c_str() << '\n';
    return std::nullopt;
  }

  return parsedArgs;
}

void setupLocalization() {
  QLocale locale = QLocale::system();
  QString langCode = locale.name();
  QString baseLang = langCode.section('_', 0, 0);

  static QTranslator translator;
  if (translator.load(QString("%1_%2.qm").arg(APP_NAME).arg(baseLang),
                      ":/i18n")) {
    QCoreApplication::installTranslator(&translator);
  } else {
    qWarning() << "Failed to load translation for" << baseLang;
  }
}

int main(int argc, char *argv[]) {
  try {
    auto parsedArgs = parseCommandLine(argc, argv);
    if (!parsedArgs.has_value()) {
      return 0;
    }

    setLogLevel(parseLogLevel(parsedArgs->operator[]("log").as<std::string>()));
    qInstallMessageHandler(messageHandler);

    qInfo() << fmt::format("Hello from {} {}!", APP_NAME, APP_VERSION).c_str();

    QApplication app(argc, argv);

    services::registerAll();
    QApplication::setApplicationName(QString::fromStdString(APP_NAME));
    QApplication::setOrganizationName(
        QString::fromStdString(ORGANIZATION_NAME));
    QApplication::setWindowIcon(QIcon(":/icons/app_icon.png"));
    QGuiApplication::setDesktopFileName(APP_ID);

    setupLocalization();

    AppMainWindow mainWindow;

    setTheme();

    if (parsedArgs->contains("smoke-test")) {
      qInfo() << "Smoke test successful: Application initialized and exiting.";
      return 0;
    }

    mainWindow.show();

    return QApplication::exec();
  } catch (const std::exception &e) {
    std::cerr << "UNCAUGHT EXCEPTION: " << e.what() << '\n';
    return 1;
  } catch (...) {
    std::cerr << "UNKNOWN EXCEPTION\n";
    return 1;
  }
}
