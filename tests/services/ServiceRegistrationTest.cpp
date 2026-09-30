// NOLINTBEGIN
#include "events/LogEvent.h"
#include "events/system/EventSystem.hpp"
#include "services/ConsoleLogService.hpp"
#include "services/registry/ServiceRegistry.hpp"

#include <QTest>
#include <gtest/gtest.h>

class ServiceRegistrationTest : public ::testing::Test {
protected:
  void SetUp() override {
    ConsoleLogService::receivedCount.store(0, std::memory_order_relaxed);
  }

  void TearDown() override {
    services::unregisterAll();
    ConsoleLogService::receivedCount.store(0, std::memory_order_relaxed);
  }
};

TEST_F(ServiceRegistrationTest, RegisterAllRetainsSubscriptionAndDelivers) {
  services::registerAll();

  events::publish(LogEvent{"service-registration-probe"});

  // EventSystem delivery is always queued (Qt::QueuedConnection).
  QTest::qWait(50);

  EXPECT_GE(ConsoleLogService::receivedCount.load(std::memory_order_relaxed), 1)
      << "ConsoleLogService did not receive LogEvent after services::registerAll()";
}

TEST_F(ServiceRegistrationTest, SubscriptionRemainsAliveAcrossMultiplePublishes) {
  services::registerAll();

  events::publish(LogEvent{"first"});
  QTest::qWait(50);
  const int afterFirst = ConsoleLogService::receivedCount.load(std::memory_order_relaxed);
  ASSERT_GE(afterFirst, 1);

  events::publish(LogEvent{"second"});
  QTest::qWait(50);
  const int afterSecond = ConsoleLogService::receivedCount.load(std::memory_order_relaxed);

  EXPECT_GE(afterSecond, afterFirst + 1)
      << "Subscription did not remain active after registerAll() returned";
}

TEST_F(ServiceRegistrationTest, UnregisterAllStopsReception) {
  services::registerAll();

  events::publish(LogEvent{"before-unregister"});
  QTest::qWait(50);
  const int afterRegister =
      ConsoleLogService::receivedCount.load(std::memory_order_relaxed);
  ASSERT_GE(afterRegister, 1);

  services::unregisterAll();

  events::publish(LogEvent{"after-unregister"});
  QTest::qWait(50);

  EXPECT_EQ(ConsoleLogService::receivedCount.load(std::memory_order_relaxed),
            afterRegister)
      << "ConsoleLogService received LogEvent after services::unregisterAll()";
}

TEST_F(ServiceRegistrationTest, RepeatedUnregisterAllIsHarmless) {
  services::registerAll();

  events::publish(LogEvent{"before-unregister"});
  QTest::qWait(50);
  const int afterRegister =
      ConsoleLogService::receivedCount.load(std::memory_order_relaxed);
  ASSERT_GE(afterRegister, 1);

  services::unregisterAll();
  services::unregisterAll();
  services::unregisterAll();

  events::publish(LogEvent{"after-unregister"});
  QTest::qWait(50);

  EXPECT_EQ(ConsoleLogService::receivedCount.load(std::memory_order_relaxed),
            afterRegister)
      << "Repeated unregisterAll() was not harmless";
}

TEST_F(ServiceRegistrationTest, ReRegisterAfterUnregisterRestoresDelivery) {
  services::registerAll();
  events::publish(LogEvent{"first-window"});
  QTest::qWait(50);
  const int afterFirst =
      ConsoleLogService::receivedCount.load(std::memory_order_relaxed);
  ASSERT_GE(afterFirst, 1);

  services::unregisterAll();
  events::publish(LogEvent{"gap"});
  QTest::qWait(50);
  ASSERT_EQ(ConsoleLogService::receivedCount.load(std::memory_order_relaxed),
            afterFirst);

  services::registerAll();
  events::publish(LogEvent{"second-window"});
  QTest::qWait(50);

  EXPECT_GE(ConsoleLogService::receivedCount.load(std::memory_order_relaxed),
            afterFirst + 1)
      << "registerAll() after unregisterAll() did not restore delivery";
}

#include "ServiceRegistrationTest.moc"

// NOLINTEND
