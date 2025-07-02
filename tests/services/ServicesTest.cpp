// NOLINTBEGIN
#include "services/registry/ServiceRegistry.hpp"
#include <QTest>
#include <gtest/gtest.h>
#include <set>

// ----------------------------
// File-scope data + handlers
// ----------------------------

struct InputEvent {
  int value;
};

struct OutputEvent {
  int value;
};

static OutputEvent handle(InputEvent inputEvent) {
  return OutputEvent{inputEvent.value};
}

static int oneWayActual = 0;
static void oneWayHandler(const InputEvent &event) {
  oneWayActual = event.value;
}

static std::optional<OutputEvent> optionalServiceHandler(const InputEvent &input) {
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

static bool serviceScopedWasCalled = false;
static void serviceScopedHandler(const InputEvent &) {
  serviceScopedWasCalled = true;
}

static OutputEvent lateSubscriberHandler(const InputEvent &e) {
  return OutputEvent{e.value};
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

// ----------------------------
// Test Fixture
// ----------------------------

class ServicesTest : public ::testing::Test {
protected:
  void SetUp() override {
    oneWayActual = 0;
    serviceScopedWasCalled = false;
  }
};

// ----------------------------
// Test Cases
// ----------------------------

TEST_F(ServicesTest, ServicesWork) {
  services::ServiceRegistry::registerService<InputEvent, OutputEvent>(handle);

  int actual = 0;
  int expected = 1;

  auto qObj = new QObject();
  events::subscribe<OutputEvent>(
      qObj, [&](const OutputEvent &event) { actual = event.value; });

  events::publish(InputEvent{expected});
  QTest::qWait(100);

  ASSERT_EQ(actual, expected);
}

TEST_F(ServicesTest, OneWayServiceFires) {
  services::ServiceRegistry::registerOneWayService<InputEvent>(oneWayHandler);

  events::publish(InputEvent{99});
  QTest::qWait(100);

  ASSERT_EQ(oneWayActual, 99);
}

TEST_F(ServicesTest, OptionalServiceOnlyPublishesWhenPopulated) {
  services::ServiceRegistry::registerOptionalService<InputEvent, OutputEvent>(optionalServiceHandler);

  int actual = -1; // sentinel
  auto qObj = new QObject();
  events::subscribe<OutputEvent>(
      qObj, [&](const OutputEvent &event) { actual = event.value; });

  events::publish(InputEvent{-1});
  QTest::qWait(100);
  ASSERT_EQ(actual, -1); // should remain unchanged

  events::publish(InputEvent{5});
  QTest::qWait(100);
  ASSERT_EQ(actual, 10);
}

TEST_F(ServicesTest, MultipleServicesCanCoexist) {
  services::ServiceRegistry::registerService<MultiInput, MultiOutput>(multiHandler1);
  services::ServiceRegistry::registerService<MultiInput, MultiOutput>(multiHandler2);

  std::set<int> seen;
  QObject receiver;
  events::subscribe<MultiOutput>(
      &receiver, [&](const MultiOutput &e) { seen.insert(e.value); });

  events::publish(MultiInput{1});
  QTest::qWait(100);

  ASSERT_NE(seen.find(2), seen.end());
  ASSERT_NE(seen.find(6), seen.end());
  ASSERT_EQ(seen.size(), 2);
}

TEST_F(ServicesTest, ServiceScopedToQCoreApplication) {
  services::ServiceRegistry::registerOneWayService<InputEvent>(serviceScopedHandler);

  events::publish(InputEvent{123});
  QTest::qWait(100);

  ASSERT_TRUE(serviceScopedWasCalled);
}

TEST_F(ServicesTest, LateSubscribersGetServiceOutput) {
  services::ServiceRegistry::registerService<InputEvent, OutputEvent>(lateSubscriberHandler);

  // publish before subscribing
  events::publish(InputEvent{7});
  QTest::qWait(100);

  int actual = 0;
  QObject listener;
  events::subscribe<OutputEvent>(
      &listener, [&](const OutputEvent &e) { actual = e.value; });

  ASSERT_EQ(actual, 0); // nothing received yet

  events::publish(InputEvent{8});
  QTest::qWait(100);

  ASSERT_EQ(actual, 8);
}

TEST_F(ServicesTest, OptionalServiceCanSkipOutput) {
  services::ServiceRegistry::registerOptionalService<QuietInput, QuietOutput>(alwaysSkipHandler);

  int actual = -1;
  QObject listener;
  events::subscribe<QuietOutput>(
      &listener, [&](const QuietOutput &e) { actual = e.value; });

  events::publish(QuietInput{999});
  QTest::qWait(100);

  ASSERT_EQ(actual, -1); // still unchanged
}

#include "ServicesTest.moc"
// NOLINTEND