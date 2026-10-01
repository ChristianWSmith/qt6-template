// NOLINTBEGIN
#include "events/LogEvent.h"
#include "events/system/EventSystem.hpp"
#include "services/registry/ServiceRegistry.hpp"

#include <QLoggingCategory>
#include <QTest>
#include <atomic>
#include <gtest/gtest.h>

namespace {

// Test-local probe: count app.service messages produced by
// ConsoleLogService::handle after services::registerAll(). Keeps production
// ConsoleLogService free of test instrumentation (F-21).
std::atomic<int> g_serviceMessages{0};
QtMessageHandler g_previousHandler = nullptr;

void countingHandler(QtMsgType type, const QMessageLogContext &context,
                     const QString &msg) {
  if (context.category != nullptr &&
      qstrcmp(context.category, "app.service") == 0) {
    g_serviceMessages.fetch_add(1, std::memory_order_relaxed);
  }
  if (g_previousHandler != nullptr) {
    g_previousHandler(type, context, msg);
  }
}

} // namespace

class ServiceRegistrationTest : public ::testing::Test {
protected:
  void SetUp() override {
    g_serviceMessages.store(0, std::memory_order_relaxed);
    g_previousHandler = qInstallMessageHandler(countingHandler);
  }

  void TearDown() override {
    services::unregisterAll();
    qInstallMessageHandler(g_previousHandler);
    g_previousHandler = nullptr;
    g_serviceMessages.store(0, std::memory_order_relaxed);
  }
};

TEST_F(ServiceRegistrationTest, RegisterAllRetainsSubscriptionAndDelivers) {
  services::registerAll();

  events::publish(LogEvent{"service-registration-probe"});

  // EventSystem delivery is always queued (Qt::QueuedConnection).
  QTest::qWait(50);

  EXPECT_GE(g_serviceMessages.load(std::memory_order_relaxed), 1)
      << "ConsoleLogService did not receive LogEvent after services::registerAll()";
}

TEST_F(ServiceRegistrationTest, SubscriptionRemainsAliveAcrossMultiplePublishes) {
  services::registerAll();

  events::publish(LogEvent{"first"});
  QTest::qWait(50);
  const int afterFirst = g_serviceMessages.load(std::memory_order_relaxed);
  ASSERT_GE(afterFirst, 1);

  events::publish(LogEvent{"second"});
  QTest::qWait(50);
  const int afterSecond = g_serviceMessages.load(std::memory_order_relaxed);

  EXPECT_GE(afterSecond, afterFirst + 1)
      << "Subscription did not remain active after registerAll() returned";
}

TEST_F(ServiceRegistrationTest, UnregisterAllStopsReception) {
  services::registerAll();

  events::publish(LogEvent{"before-unregister"});
  QTest::qWait(50);
  const int afterRegister = g_serviceMessages.load(std::memory_order_relaxed);
  ASSERT_GE(afterRegister, 1);

  services::unregisterAll();

  events::publish(LogEvent{"after-unregister"});
  QTest::qWait(50);

  EXPECT_EQ(g_serviceMessages.load(std::memory_order_relaxed), afterRegister)
      << "ConsoleLogService received LogEvent after services::unregisterAll()";
}

TEST_F(ServiceRegistrationTest, RepeatedUnregisterAllIsHarmless) {
  services::registerAll();

  events::publish(LogEvent{"before-unregister"});
  QTest::qWait(50);
  const int afterRegister = g_serviceMessages.load(std::memory_order_relaxed);
  ASSERT_GE(afterRegister, 1);

  services::unregisterAll();
  services::unregisterAll();
  services::unregisterAll();

  events::publish(LogEvent{"after-unregister"});
  QTest::qWait(50);

  EXPECT_EQ(g_serviceMessages.load(std::memory_order_relaxed), afterRegister)
      << "Repeated unregisterAll() was not harmless";
}

TEST_F(ServiceRegistrationTest, ReRegisterAfterUnregisterRestoresDelivery) {
  services::registerAll();
  events::publish(LogEvent{"first-window"});
  QTest::qWait(50);
  const int afterFirst = g_serviceMessages.load(std::memory_order_relaxed);
  ASSERT_GE(afterFirst, 1);

  services::unregisterAll();
  events::publish(LogEvent{"gap"});
  QTest::qWait(50);
  ASSERT_EQ(g_serviceMessages.load(std::memory_order_relaxed), afterFirst);

  services::registerAll();
  events::publish(LogEvent{"second-window"});
  QTest::qWait(50);

  EXPECT_GE(g_serviceMessages.load(std::memory_order_relaxed), afterFirst + 1)
      << "registerAll() after unregisterAll() did not restore delivery";
}

#include "ServiceRegistrationTest.moc"

// NOLINTEND
