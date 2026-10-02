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
#include <QScopeGuard>
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

// Runtime resource embedding check for --smoke-test (E-002).
// Complements cmake/VerifyProductionResources.cmake hex-scan by exercising
// the real Qt resource system in the production binary.
bool verifyProductionResources() {
  const QStringList required = {
      QStringLiteral(":/icons/app_icon.png"),
      QStringLiteral(":/styles/custom.qss"),
#if defined(Q_OS_WIN)
      QStringLiteral(":/styles/windows/dark.qss"),
      QStringLiteral(":/styles/windows/light.qss"),
#elif defined(Q_OS_MACOS)
      QStringLiteral(":/styles/macos/base.qss"),
#else
      QStringLiteral(":/styles/linux/base.qss"),
#endif
  };
  bool ok = true;
  for (const QString &path : required) {
    if (!QFile::exists(path)) {
      qCCritical(appMain) << "Smoke test: missing embedded resource:" << path;
      ok = false;
    }
  }
  return ok;
}

int main(int argc, char *argv[]) {
  try {
    // CLI parsing and configureLogLevel (logging module) live behind the
    // same try/catch so option errors are classified separately from
    // post-register bootstrap failures.
    cxxopts::Options options(APP_NAME, APP_DESCRIPTION);
    options.add_options()("l,log", "Log level (debug, info, warn, error, none)",
                          cxxopts::value<std::string>()->default_value("info"))(
        "smoke-test", "Run in smoke test mode (exits immediately after setup)")(
        "h,help", "Print help");

    cxxopts::ParseResult parsedArgs = options.parse(argc, argv);
    if (parsedArgs.contains("help")) {
      std::cout << options.help().c_str() << '\n';
      return 0;
    }

    // Custom message handler retained deliberately (pedagogy + fatal/sink
    // behavior); see logging.cpp. qSetMessagePattern is the formatting-only
    // alternative.
    qInstallMessageHandler(messageHandler);
    configureLogLevel(parsedArgs.operator[]("log").as<std::string>());

    // fmt demo (Keep list): Hello message uses fmt; logging.cpp uses fmt too.
    qCInfo(appMain)
        << fmt::format("Hello from {} {}!", APP_NAME, APP_VERSION).c_str();

    QApplication app(argc, argv);

    // App metadata immediately after QApplication so every later path
    // (persistence, QSettings, AppDataLocation) sees correct identity.
    // Invariant: no persistence or QSettings use before this block.
    QApplication::setApplicationName(QString::fromUtf8(APP_NAME));
    QApplication::setOrganizationName(QString::fromUtf8(ORGANIZATION_NAME));
    QApplication::setWindowIcon(QIcon(":/icons/app_icon.png"));
    QGuiApplication::setDesktopFileName(APP_ID);

    // Visible lifecycle API remains services::registerAll()/unregisterAll().
    // qScopeGuard is structural backup so exception unwind after registration
    // cannot skip unregister. Explicit unregisterAll() calls below stay for
    // readability; they are idempotent.
    // Note: on exception paths C++ destroys automatics (including this guard)
    // BEFORE catch handlers run — the guard is the exception-path mechanism.
    // Catch-block unregisterAll() calls below are idempotent no-ops after
    // ~QApplication (Subscription::reset disconnects only).
    services::registerAll();
    const auto unregisterServicesGuard =
        qScopeGuard([] { services::unregisterAll(); });

    // Declared after QApplication so it is destroyed before the application
    // on scope exit. qScopeGuard removes the translator on every path after
    // setup (success, smoke, exception unwind) so QCoreApplication never
    // observes a destroyed translator. Explicit removeTranslator calls below
    // stay for readability — they document intent on the normal paths and
    // are idempotent; catch paths rely on the guard alone.
    QTranslator translator;
    setupLocalization(translator);
    const auto translatorGuard = qScopeGuard(
        [&translator] { QCoreApplication::removeTranslator(&translator); });

    // Structural provider lifetime on the production path (AUD-001 Option B):
    // provider declared BEFORE the window so it outlives it (stack order;
    // destruction is reverse of declaration). Injected ctor binds the
    // caller-owned provider; the window does not reparent it.
    FilePersistenceProvider provider;
    AppMainWindow mainWindow(provider);

    setTheme();

    if (parsedArgs.contains("smoke-test")) {
      // May READ real AppDataLocation on developer machines (model ctors
      // call loadState); does not save on this path.
      if (!verifyProductionResources()) {
        services::unregisterAll();
        QCoreApplication::removeTranslator(&translator);
        return 1;
      }
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
    // Idempotent after unwind (guard already ran during stack unwinding
    // before this handler; ~QApplication has already destroyed the app).
    services::unregisterAll();
    std::cerr << "UNCAUGHT EXCEPTION: " << e.what() << '\n';
    return 1;
  } catch (...) {
    services::unregisterAll();
    std::cerr << "UNKNOWN EXCEPTION\n";
    return 1;
  }
}
