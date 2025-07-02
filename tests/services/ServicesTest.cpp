// NOLINTBEGIN
#include "services/registry/ServiceRegistry.hpp"
#include <QTest>
#include <gtest/gtest.h>
#include <set>

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

TEST_F(ServicesTest, OneWayServiceFires) {
  int actual = 0;

  services::ServiceRegistry::registerOneWayService<InputEvent>(
      [&](const InputEvent &event) { actual = event.value; });

  events::publish(InputEvent{99});
  QTest::qWait(1);

  ASSERT_EQ(actual, 99);
}

TEST_F(ServicesTest, OptionalServiceOnlyPublishesWhenPopulated) {
  services::ServiceRegistry::registerOptionalService<InputEvent, OutputEvent>(
      [](const InputEvent &input) -> std::optional<OutputEvent> {
        if (input.value > 0)
          return OutputEvent{input.value * 2};
        return std::nullopt;
      });

  int actual = -1; // Set to sentinel value to detect unwanted change
  auto qObj = new QObject();
  events::subscribe<OutputEvent>(
      qObj, [&](const OutputEvent &event) { actual = event.value; });

  events::publish(InputEvent{-1});
  QTest::qWait(1);
  ASSERT_EQ(actual, -1); // Should remain unchanged

  events::publish(InputEvent{5});
  QTest::qWait(1);
  ASSERT_EQ(actual, 10);
}

TEST_F(ServicesTest, MultipleServicesCanCoexist) {
  struct MultiInput {
    int value;
  };
  struct MultiOutput {
    int value;
  };

  int a = 0;
  int b = 0;

  services::ServiceRegistry::registerService<MultiInput, MultiOutput>(
      [](const MultiInput &e) { return MultiOutput{e.value + 1}; });

  services::ServiceRegistry::registerService<MultiInput, MultiOutput>(
      [](const MultiInput &e) { return MultiOutput{e.value + 5}; });

  std::set<int> seen;
  QObject receiver;
  events::subscribe<MultiOutput>(
      &receiver, [&](const MultiOutput &e) { seen.insert(e.value); });

  events::publish(MultiInput{1});
  QTest::qWait(1);

  ASSERT_NE(seen.find(2), seen.end());
  ASSERT_NE(seen.find(6), seen.end());
  ASSERT_EQ(seen.size(), 2);
}

TEST_F(ServicesTest, ServiceScopedToQCoreApplication) {
  bool wasCalled = false;

  services::ServiceRegistry::registerOneWayService<InputEvent>(
      [&](const InputEvent &) { wasCalled = true; });

  events::publish(InputEvent{123});
  QTest::qWait(1);

  ASSERT_TRUE(wasCalled);
}

TEST_F(ServicesTest, LateSubscribersGetServiceOutput) {
  services::ServiceRegistry::registerService<InputEvent, OutputEvent>(
      [](const InputEvent &e) { return OutputEvent{e.value}; });

  // Publish BEFORE subscribing
  events::publish(InputEvent{7});
  QTest::qWait(1);

  int actual = 0;
  QObject listener;
  events::subscribe<OutputEvent>(
      &listener, [&](const OutputEvent &e) { actual = e.value; });

  // Still 0 because no event has occurred since subscribing
  ASSERT_EQ(actual, 0);

  // Now publish something that should be received
  events::publish(InputEvent{8});
  QTest::qWait(1);
  ASSERT_EQ(actual, 8);
}

TEST_F(ServicesTest, OptionalServiceCanSkipOutput) {
  struct QuietInput {
    int value;
  };
  struct QuietOutput {
    int value;
  };

  services::ServiceRegistry::registerOptionalService<QuietInput, QuietOutput>(
      [](const QuietInput &) -> std::optional<QuietOutput> {
        return std::nullopt; // Always skips
      });

  int actual = -1;
  QObject listener;
  events::subscribe<QuietOutput>(
      &listener, [&](const QuietOutput &e) { actual = e.value; });

  events::publish(QuietInput{999});
  QTest::qWait(1);

  ASSERT_EQ(actual, -1); // Still unchanged
}

#include "ServicesTest.moc"
// NOLINTEND
