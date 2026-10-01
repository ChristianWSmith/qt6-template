// NOLINTBEGIN
#include "gtest/gtest.h"
#include <QApplication>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>

QDebug operator<<(QDebug debug, const std::string &str) {
  debug.noquote() << QString::fromStdString(str);
  return debug;
}

QDebug operator<<(QDebug debug, std::string_view sv) {
  debug.noquote() << QString::fromUtf8(sv.data(), int(sv.size()));
  return debug;
}

int main(int argc, char **argv) {
  // Redirect QStandardPaths (AppDataLocation, etc.) into Qt's test mode
  // before any test constructs persistence providers or windows.
  QStandardPaths::setTestModeEnabled(true);

  // QStandardPaths test mode does not redirect QSettings. Point UserScope
  // storage at a temp dir so LifecycleTest cannot touch the developer's
  // real organization/application settings.
  // NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
  static QTemporaryDir settingsDir;
  // NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)
  if (settingsDir.isValid()) {
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope,
                       settingsDir.path());
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       settingsDir.path());
  }

  QApplication a(argc, argv);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
// NOLINTEND
