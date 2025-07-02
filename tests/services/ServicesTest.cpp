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

// === Shared service handlers and state ===

static int g_oneWayActual = 0;
static int g_servicesWorkActual = 0;
static int g_optionalActual = -1;
static int g_lateSubActual = 0;
static int g_quietActual = -1;

static OutputEvent handleInputToOutput(InputEvent inputEvent) {
  return OutputEvent{inputEvent.value};
}

static void handleOneWayInput(const InputEvent &event) {
  g_oneWayActual = event.value;
}

static std::optional<OutputEvent> handleOptionalInput(const InputEvent &input) {
  if (input.value > 0)
    return OutputEvent{input.value * 2};
  return std::nullopt;
}

static OutputEvent handleLateSubInput(const InputEvent &e) {
  return OutputEvent{e.value};
}

struct ServicesTest : public ::testing::Test {
protected:
  void SetUp() override {
    g_oneWayActual = 0;
    g_servicesWorkActual = 0;
    g_optionalActual = -1;
    g_lateSubActual = 0;
    g_quietActual = -1;
  }
};

// === TESTS ===

TEST_F(ServicesTest, ServicesWork) {
  services::ServiceRegistry::registerService<InputEvent, OutputEvent>(handleInputToOutput);

  auto qObj = new QObject();
  events::subscribe<OutputEvent>(
      qObj, [](const OutputEvent &event) { g_servicesWorkActual = event.value; });

  events::publish(InputEvent{1});
  QTest::qWait(100);

  ASSERT_EQ(g_servicesWorkActual, 1);
}

TEST_F(ServicesTest, OneWayServiceFires) {
  services::ServiceRegistry::registerOneWayService<InputEvent>(handleOneWayInput);

  events::publish(InputEvent{99});
  QTest::qWait(100);

  ASSERT_EQ(g_oneWayActual, 99);
}

TEST_F(ServicesTest, OptionalServiceOnlyPublishesWhenPopulated) {
  services::ServiceRegistry::registerOptionalService<InputEvent, OutputEvent>(handleOptionalInput);

  auto qObj = new QObject();
  events::subscribe<OutputEvent>(
      qObj, [](const OutputEvent &event) { g_optionalActual = event.value; });

  events::publish(InputEvent{-1});
  QTest::qWait(100);
  ASSERT_EQ(g_optionalActual, -1); // Should remain unchanged

  events::publish(InputEvent{5});
  QTest::qWait(100);
  ASSERT_EQ(g_optionalActual, 10);
}

TEST_F(ServicesTest, MultipleServicesCanCoexist) {
  struct MultiInput {
    int value;
  };
  struct MultiOutput {
    int value;
  };

  static std::set<int> seen;

  seen.clear();
  static MultiOutput multiHandler1(const MultiInput &e) {
    return MultiOutput{e.value + 1};
  }

  static MultiOutput multiHandler2(const MultiInput &e) {
    return MultiOutput{e.value + 5};
  }

  services::ServiceRegistry::registerService<MultiInput, MultiOutput>(multiHandler1);
  services::ServiceRegistry::registerService<MultiInput, MultiOutput>(multiHandler2);

  QObject receiver;
  events::subscribe<MultiOutput>(
      &receiver, [](const MultiOutput &e) { seen.insert(e.value); });

  events::publish(MultiInput{1});
  QTest::qWait(100);

  ASSERT_NE(seen.find(2), seen.end());
  ASSERT_NE(seen.find(6), seen.end());
  ASSERT_EQ(seen.size(), 2);
}

TEST_F(ServicesTest, ServiceScopedToQCoreApplication) {
  services::ServiceRegistry::registerOneWayService<InputEvent>(handleOneWayInput);

  events::publish(InputEvent{123});
  QTest::qWait(100);

  ASSERT_EQ(g_oneWayActual, 123);
}

TEST_F(ServicesTest, LateSubscribersGetServiceOutput) {
  services::ServiceRegistry::registerService<InputEvent, OutputEvent>(handleLateSubInput);

  // Publish BEFORE subscribing
  events::publish(InputEvent{7});
  QTest::qWait(100);

  QObject listener;
  events::subscribe<OutputEvent>(
      &listener, [](const OutputEvent &e) { g_lateSubActual = e.value; });

  // Should still be zero
  ASSERT_EQ(g_lateSubActual, 0);

  events::publish(InputEvent{8});
  QTest::qWait(100);
  ASSERT_EQ(g_lateSubActual, 8);
}

TEST_F(ServicesTest, OptionalServiceCanSkipOutput) {
  struct QuietInput {
    int value;
  };
  struct QuietOutput {
    int value;
  };

  static std::optional<QuietOutput> alwaysSkipHandler(const QuietInput &) {
    return std::nullopt;
  }

  services::ServiceRegistry::registerOptionalService<QuietInput, QuietOutput>(alwaysSkipHandler);

  QObject listener;
  events::subscribe<QuietOutput>(
      &listener, [](const QuietOutput &e) { g_quietActual = e.value; });

  events::publish(QuietInput{999});
  QTest::qWait(100);

  ASSERT_EQ(g_quietActual, -1); // Still unchanged
}

#include "ServicesTest.moc"
// NOLINTEND