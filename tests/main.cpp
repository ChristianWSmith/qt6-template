// NOLINTBEGIN
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <QApplication>
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
  QApplication a(argc, argv);

  ::testing::InitGoogleMock(&argc, argv);
  return RUN_ALL_TESTS();
}
// NOLINTEND
