#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <QApplication>
#include <gtest/gtest.h>

int main(int argc, char **argv) {
  QApplication a(argc, argv);

  ::testing::InitGoogleMock(&argc, argv);
  return RUN_ALL_TESTS();
}
