
// NOLINTBEGIN
#include "services/registry/ServiceRegistry.hpp"
#include <QTest>
#include <gtest/gtest.h>

struct InputEvent {
  int value;
};

struct OutputEvent {
  int value;
};

OutputEvent handle(InputEvent inputEvent) {
  return OutputEvent{inputEvent.value};
};

class ServicesTest : public ::testing::Test {
protected:
  ServicesTest() {}
};

TEST_F(ServicesTest, ServicesWork) {
  services::ServiceRegistry::registerService<InputEvent, OutputEvent>(handle);

  int actual = 0;
  int expected = 1;

  auto qObj = new QObject();
  events::subscribe<OutputEvent>(
      qObj, [&](const OutputEvent &event) { actual = event.value; });

  events::publish(InputEvent{expected});
  QTest::qWait(1);

  ASSERT_EQ(actual, expected);
}

#include "ServicesTest.moc"

// NOLINTEND
