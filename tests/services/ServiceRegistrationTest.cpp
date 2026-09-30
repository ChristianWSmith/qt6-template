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

#include "ServiceRegistrationTest.moc"

// NOLINTEND
