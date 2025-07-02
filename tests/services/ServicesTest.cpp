// NOLINTBEGIN
#include "services/registry/ServiceRegistry.hpp"
#include <QTest>
#include <gtest/gtest.h>
#include <optional>
#include <set>

struct InputEvent {
  int value;
};

struct OutputEvent {
  int value;
};

static OutputEvent handle(const InputEvent &event) {
  return OutputEvent{event.value};
}

static void oneWayHandler(const InputEvent &event) {
  extern int oneWayActualValue;
  oneWayActualValue = event.value;
}

static std::optional<OutputEvent>
optionalServiceHandler(const InputEvent &input) {
  if (input.value > 0)
    return OutputEvent{input.value * 2};
  return std::nullopt;
}

struct MultiInput {
  int value;
};

struct MultiOutput {
  int value;
};

static MultiOutput multiHandler1(const MultiInput &e) {
  return MultiOutput{e.value + 1};
}

static MultiOutput multiHandler2(const MultiInput &e) {
  return MultiOutput{e.value + 5};
}

struct QuietInput {
  int value;
};

struct QuietOutput {
  int value;
};

static std::optional<QuietOutput> alwaysSkipHandler(const QuietInput &) {
  return std::nullopt;
}

// Globals for test state mutation
int oneWayActualValue = 0;
bool wasServiceCalled = false;
int lateSubscriberValue = 0;
int quietOutputValue = -1;

class ServicesTest : public ::testing::Test {
protected:
  ServicesTest() {
    oneWayActualValue = 0;
    wasServiceCalled = false;
    lateSubscriberValue = 0;
    quietOutputValue = -1;
  }
};

TEST_F(ServicesTest, ServicesWork) {
  services::ServiceRegistry::registerService<InputEvent, OutputEvent>(handle);

  int actual = 0;
  int expected = 1;

  QObject qObj;
  events::subscribe<OutputEvent>(
      &qObj, [&](const OutputEvent &event) { actual = event.value; });

  events::publish(InputEvent{expected});
  QTest::qWait(1);

  ASSERT_EQ(actual, expected);
}

TEST_F(ServicesTest, OneWayServiceFires) {
  services::ServiceRegistry::registerOneWayService<InputEvent>(oneWayHandler);

  events::publish(InputEvent{99});
  QTest::qWait(1);

  ASSERT_EQ(oneWayActualValue, 99);
}

TEST_F(ServicesTest, OptionalServiceOnlyPublishesWhenPopulated) {
  services::ServiceRegistry::registerOptionalService<InputEvent, OutputEvent>(
      optionalServiceHandler);

  int actual = -1;
  QObject qObj;
  events::subscribe<OutputEvent>(
      &qObj, [&](const OutputEvent &event) { actual = event.value; });

  events::publish(InputEvent{-1});
  QTest::qWait(1);
  ASSERT_EQ(actual, -1); // Should remain unchanged

  events::publish(InputEvent{5});
  QTest::qWait(1);
  ASSERT_EQ(actual, 10);
}

TEST_F(ServicesTest, MultipleServicesCanCoexist) {
  services::ServiceRegistry::registerService<MultiInput, MultiOutput>(
      multiHandler1);
  services::ServiceRegistry::registerService<MultiInput, MultiOutput>(
      multiHandler2);

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

static void markServiceCalled(const InputEvent &) { wasServiceCalled = true; }

TEST_F(ServicesTest, ServiceScopedToQCoreApplication) {
  services::ServiceRegistry::registerOneWayService<InputEvent>(
      markServiceCalled);

  ASSERT_TRUE(QCoreApplication::instance());

  events::publish(InputEvent{123});
  QTest::qWait(1);

  ASSERT_TRUE(wasServiceCalled);
}

static OutputEvent echoHandler(const InputEvent &e) {
  return OutputEvent{e.value};
}

TEST_F(ServicesTest, LateSubscribersGetServiceOutput) {
  services::ServiceRegistry::registerService<InputEvent, OutputEvent>(
      echoHandler);

  events::publish(InputEvent{7});
  QTest::qWait(1);

  QObject listener;
  events::subscribe<OutputEvent>(
      &listener, [&](const OutputEvent &e) { lateSubscriberValue = e.value; });

  ASSERT_EQ(lateSubscriberValue, 0);

  events::publish(InputEvent{8});
  QTest::qWait(1);
  ASSERT_EQ(lateSubscriberValue, 8);
}

TEST_F(ServicesTest, OptionalServiceCanSkipOutput) {
  services::ServiceRegistry::registerOptionalService<QuietInput, QuietOutput>(
      alwaysSkipHandler);

  QObject listener;
  events::subscribe<QuietOutput>(
      &listener, [&](const QuietOutput &e) { quietOutputValue = e.value; });

  events::publish(QuietInput{999});
  QTest::qWait(1);

  ASSERT_EQ(quietOutputValue, -1); // Still unchanged
}

#include "ServicesTest.moc"
// NOLINTEND