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

// NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays, cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
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
// NOLINTEND(cppcoreguidelines-avoid-c-arrays, cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

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

void configureLogLevel(const std::string &levelStr) {
  if (levelStr.empty()) {
    return;
  }

  QStringList rules;
  if (levelStr == "debug") {
    rules << "*.debug=true" << "*.info=true" << "*.warning=true"
          << "*.critical=true";
  } else if (levelStr == "info") {
    rules << "*.debug=false" << "*.info=true" << "*.warning=true"
          << "*.critical=true";
  } else if (levelStr == "warn" || levelStr == "warning") {
    rules << "*.debug=false" << "*.info=false" << "*.warning=true"
          << "*.critical=true";
  } else if (levelStr == "error") {
    rules << "*.debug=false" << "*.info=false" << "*.warning=false"
          << "*.critical=true";
  } else if (levelStr == "none") {
    rules << "*.debug=false" << "*.info=false" << "*.warning=false"
          << "*.critical=false";
  }
  QLoggingCategory::setFilterRules(rules.join('\n'));
}

int main(int argc, char *argv[]) {
  try {
    auto parsedArgs = parseCommandLine(argc, argv);
    if (!parsedArgs.has_value()) {
      return 0;
    }

    qInstallMessageHandler(messageHandler);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
    configureLogLevel(parsedArgs->operator[]("log").as<std::string>());

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

    if (parsedArgs->contains("smoke-test")) {
      qCInfo(appMain)
          << "Smoke test successful: Application initialized and exiting.";
      QCoreApplication::removeTranslator(&translator);
      return 0;
    }

    mainWindow.show();

    const int exitCode = QApplication::exec();
    QCoreApplication::removeTranslator(&translator);
    return exitCode;
  } catch (const std::exception &e) {
    std::cerr << "UNCAUGHT EXCEPTION: " << e.what() << '\n';
    return 1;
  } catch (...) {
    std::cerr << "UNKNOWN EXCEPTION\n";
    return 1;
  }
}
