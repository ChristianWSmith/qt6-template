// NOLINTBEGIN
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <QApplication>
#include <QStandardPaths>
#include <gtest/gtest.h>

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

  QApplication a(argc, argv);

  ::testing::InitGoogleMock(&argc, argv);
  return RUN_ALL_TESTS();
}
// NOLINTEND
