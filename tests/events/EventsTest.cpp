// NOLINTBEGIN
#include "events/system/EventSystem.hpp"
#include <QCoreApplication>
#include <QLoggingCategory>
#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTest>
#include <QVariant>
#include <atomic>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

struct Event {
  int value;
};

struct EventA {
  int val;
};

struct EventB {
  int val;
};

struct Event2 {
  QString message;
};

class EventTest : public ::testing::Test {
protected:
  EventTest() = default;
};

// --- Helpers for moving state into QObject receivers ---

class IntReceiver : public QObject {
  Q_OBJECT
public:
  int *target;
  explicit IntReceiver(int *t) : target(t) {}

public slots:
  void receive(const Event &e) { *target = e.value; }
};

class Event2Receiver : public QObject {
  Q_OBJECT
public:
  QString *target;
  explicit Event2Receiver(QString *t) : target(t) {}

public slots:
  void receive(const Event2 &e) { *target = e.message; }
};

class SummingReceiver : public QObject {
  Q_OBJECT
public:
  int *sum;
  explicit SummingReceiver(int *s) : sum(s) {}

public slots:
  void add(const Event &e) { *sum += e.value; }
};

class ChainedSubscriber : public QObject {
  Q_OBJECT
public:
  QObject *target;
  int *a;
  int *b;
  explicit ChainedSubscriber(QObject *t, int *a_, int *b_)
      : target(t), a(a_), b(b_) {}

public slots:
  void first(const Event &) {
    *a = 1;
    // Receiver-lifetime handle; discarded intentionally (connection follows
    // this QObject). Captured only to satisfy [[nodiscard]].
    [[maybe_unused]] const auto sub =
        events::subscribe<Event>(this, &ChainedSubscriber::second);
  }

  void second(const Event &) { *b = 1; }
};

class SelfUnsubscriber : public QObject {
  Q_OBJECT
public:
  int *calls;
  explicit SelfUnsubscriber(int *c) : calls(c) {}

public slots:
  void fire(const Event &) {
    ++(*calls);
    deleteLater();
  }
};

class Counter : public QObject {
  Q_OBJECT
public:
  int *ptr;
  explicit Counter(int *p) : ptr(p) {}

public slots:
  void count(const Event &) { ++(*ptr); }
};

class MultiReceiver : public QObject {
  Q_OBJECT
public:
  int *a;
  int *b;
  MultiReceiver(int *a_, int *b_) : a(a_), b(b_) {}

public slots:
  void recvA(const EventA &e) { *a = e.val; }
  void recvB(const EventB &e) { *b = e.val; }
};

class Bridge : public QObject {
  Q_OBJECT
public slots:
  void forward(const Event &e) {
    events::publish(Event2{QString("Value was %1").arg(e.value)});
  }
};

class ValueCollector : public QObject {
  Q_OBJECT
public:
  std::vector<int> *values;
  explicit ValueCollector(std::vector<int> *v) : values(v) {}

public slots:
  void collect(const Event &e) { values->push_back(e.value); }
};

// AUD-120: records subscription order across distinct receivers.
class OrderRecorder : public QObject {
  Q_OBJECT
public:
  std::vector<int> *order;
  int id;
  OrderRecorder(std::vector<int> *o, int i) : order(o), id(i) {}

public slots:
  void record(const Event &) { order->push_back(id); }
};

// --- Free-function helpers ---

static int g_freeValue = 0;

static void freeHandler(const Event &e) { g_freeValue = e.value; }

static int g_freeCallCount = 0;

static void freeCountHandler(const Event &) { ++g_freeCallCount; }

// AUD-101: worker-thread rejection probe.
static std::atomic<int> g_wrongThreadHits{0};

static void wrongThreadHandler(const Event &) {
  g_wrongThreadHits.fetch_add(1, std::memory_order_relaxed);
}

// AUD-004: quit-lifecycle probe (subprocess only).
static int g_quitProbeHits = 0;

static void quitProbeHandler(const Event &) { ++g_quitProbeHits; }

// AUD-101: qCCritical(app.event) message-handler probe.
// Pattern copied from ServiceRegistrationTest (countingHandler).
static std::atomic<int> g_eventCriticals{0};
static QtMessageHandler g_previousHandler = nullptr;

void eventCriticalProbe(QtMsgType type, const QMessageLogContext &context,
                        const QString &msg) {
  if (type == QtCriticalMsg && context.category != nullptr &&
      qstrcmp(context.category, "app.event") == 0) {
    g_eventCriticals.fetch_add(1, std::memory_order_relaxed);
  }
  if (g_previousHandler != nullptr) {
    g_previousHandler(type, context, msg);
  }
}

// --- TESTS ---

TEST_F(EventTest, EventsWork) {
  int actual = 0;
  int expected = 1;

  IntReceiver r(&actual);
  [[maybe_unused]] const auto sub =
      events::subscribe<Event>(&r, &IntReceiver::receive);

  events::publish(Event{expected});
  QTest::qWait(1);

  ASSERT_EQ(actual, expected);
}

TEST_F(EventTest, DestroyedSubscriberDoesNotReceiveEvents) {
  int actual = 0;

  {
    IntReceiver r(&actual);
    [[maybe_unused]] const auto sub =
        events::subscribe<Event>(&r, &IntReceiver::receive);
  }

  events::publish(Event{123});
  QTest::qWait(1);

  ASSERT_EQ(actual, 0);
}

TEST_F(EventTest, MultipleSubscribersReceiveEvents) {
  int a = 0, b = 0;
  IntReceiver ra(&a), rb(&b);

  [[maybe_unused]] const auto subA =
      events::subscribe<Event>(&ra, &IntReceiver::receive);
  [[maybe_unused]] const auto subB =
      events::subscribe<Event>(&rb, &IntReceiver::receive);

  events::publish(Event{42});
  QTest::qWait(1);

  ASSERT_EQ(a, 42);
  ASSERT_EQ(b, 42);
}

TEST_F(EventTest, SubscriberCanPublishAnotherEvent) {
  QString actualMessage;

  Bridge bridge;
  Event2Receiver recv(&actualMessage);

  [[maybe_unused]] const auto subBridge =
      events::subscribe<Event>(&bridge, &Bridge::forward);
  [[maybe_unused]] const auto subRecv =
      events::subscribe<Event2>(&recv, &Event2Receiver::receive);

  events::publish(Event{7});
  QTest::qWait(10);

  ASSERT_EQ(actualMessage, "Value was 7");
}

TEST_F(EventTest, RapidFireEventsAllHandled) {
  constexpr int count = 100;
  int sum = 0;

  SummingReceiver receiver(&sum);
  [[maybe_unused]] const auto sub =
      events::subscribe<Event>(&receiver, &SummingReceiver::add);

  for (int i = 0; i < count; ++i)
    events::publish(Event{1});

  QTest::qWait(1);
  ASSERT_EQ(sum, count);
}

TEST_F(EventTest, LambdaLifetimeTiedToOwner) {
  int actual = 0;

  {
    IntReceiver r(&actual);
    [[maybe_unused]] const auto sub =
        events::subscribe<Event>(&r, &IntReceiver::receive);
  }

  events::publish(Event{999});
  QTest::qWait(1);

  ASSERT_EQ(actual, 0);
}

TEST_F(EventTest, SubscriberCanRegisterAnotherSubscriberMidDispatch) {
  int a = 0, b = 0;

  QObject mid;
  ChainedSubscriber c(&mid, &a, &b);

  [[maybe_unused]] const auto sub =
      events::subscribe<Event>(&c, &ChainedSubscriber::first);

  events::publish(Event{});
  QTest::qWait(1);
  ASSERT_EQ(a, 1);
  ASSERT_EQ(b, 0);

  events::publish(Event{});
  QTest::qWait(1);
  ASSERT_EQ(b, 1);
}

TEST_F(EventTest, SubscriberCanUnsubscribeItselfSafely) {
  int calls = 0;
  auto obj = new SelfUnsubscriber(&calls);
  auto ptr = new QPointer(obj);

  [[maybe_unused]] const auto sub =
      events::subscribe<Event>(obj, &SelfUnsubscriber::fire);

  events::publish(Event{});
  QTest::qWait(1);
  ASSERT_TRUE(ptr->isNull());

  events::publish(Event{});
  QTest::qWait(1);
  ASSERT_EQ(calls, 1);
}

TEST_F(EventTest, ThousandsOfSubscribersAllFire) {
  constexpr int count = 1000;
  int hits = 0;

  std::vector<std::unique_ptr<Counter>> all;
  all.reserve(count);

  for (int i = 0; i < count; ++i) {
    auto c = std::make_unique<Counter>(&hits);
    [[maybe_unused]] const auto sub =
        events::subscribe<Event>(c.get(), &Counter::count);
    all.push_back(std::move(c));
  }

  events::publish(Event{});
  QTest::qWait(1);

  ASSERT_EQ(hits, count);
}

TEST_F(EventTest, OneObjectCanSubscribeToMultipleEvents) {
  int a = 0, b = 0;
  MultiReceiver m(&a, &b);

  [[maybe_unused]] const auto subA =
      events::subscribe<EventA>(&m, &MultiReceiver::recvA);
  [[maybe_unused]] const auto subB =
      events::subscribe<EventB>(&m, &MultiReceiver::recvB);

  events::publish(EventA{1});
  events::publish(EventB{2});
  QTest::qWait(1);

  ASSERT_EQ(a, 1);
  ASSERT_EQ(b, 2);
}

TEST_F(EventTest, FreeFunctionSubscriptionReceivesEvents) {
  g_freeValue = 0;
  events::Subscription sub = events::subscribe<Event>(freeHandler);

  events::publish(Event{77});
  QTest::qWait(1);

  ASSERT_EQ(g_freeValue, 77);
}

TEST_F(EventTest, FreeFunctionSubscriptionStopsOnDestruction) {
  g_freeValue = 0;

  {
    events::Subscription sub = events::subscribe<Event>(freeHandler);
    events::publish(Event{1});
    QTest::qWait(1);
    ASSERT_EQ(g_freeValue, 1);
  }

  g_freeValue = 0;
  events::publish(Event{99});
  QTest::qWait(1);

  ASSERT_EQ(g_freeValue, 0);
}

TEST_F(EventTest, MultipleFreeFunctionSubscriptions) {
  g_freeValue = 0;
  g_freeCallCount = 0;

  events::Subscription sub1 = events::subscribe<Event>(freeHandler);
  events::Subscription sub2 = events::subscribe<Event>(freeCountHandler);

  events::publish(Event{55});
  QTest::qWait(1);

  ASSERT_EQ(g_freeValue, 55);
  ASSERT_EQ(g_freeCallCount, 1);
}

TEST_F(EventTest, SubscriptionMoveTransfersOwnership) {
  g_freeValue = 0;

  events::Subscription sub1 = events::subscribe<Event>(freeHandler);
  events::Subscription sub2 = std::move(sub1);

  ASSERT_FALSE(sub1.isConnected());
  ASSERT_TRUE(sub2.isConnected());

  events::publish(Event{42});
  QTest::qWait(1);

  ASSERT_EQ(g_freeValue, 42);

  sub2.reset();
  g_freeValue = 0;
  events::publish(Event{100});
  QTest::qWait(1);

  ASSERT_EQ(g_freeValue, 0);
}

// Note: after QCoreApplication teardown Qt invalidates connection handles;
// Subscription reset()/~Subscription() are safe no-ops (inert) on such
// handles — they do not depend on deleteLater().
TEST_F(EventTest, SubscriptionResetStopsDelivery) {
  g_freeValue = 0;

  events::Subscription sub = events::subscribe<Event>(freeHandler);
  events::publish(Event{10});
  QTest::qWait(1);
  ASSERT_EQ(g_freeValue, 10);

  sub.reset();
  ASSERT_FALSE(sub.isConnected());

  g_freeValue = 0;
  events::publish(Event{20});
  QTest::qWait(1);
  ASSERT_EQ(g_freeValue, 0);
}

TEST_F(EventTest, RepeatedSubscribeUnsubscribe) {
  g_freeValue = 0;

  for (int i = 0; i < 10; ++i) {
    events::Subscription sub = events::subscribe<Event>(freeHandler);
    events::publish(Event{i});
    QTest::qWait(1);
    ASSERT_EQ(g_freeValue, i);
  }
}

TEST_F(EventTest, SubscriptionIsConnected) {
  events::Subscription empty;
  ASSERT_FALSE(empty.isConnected());

  events::Subscription sub = events::subscribe<Event>(freeHandler);
  ASSERT_TRUE(sub.isConnected());

  sub.reset();
  ASSERT_FALSE(sub.isConnected());
}

TEST_F(EventTest, SubscriptionMoveAssignmentTransfersOwnership) {
  g_freeValue = 0;
  g_freeCallCount = 0;

  events::Subscription sub1 = events::subscribe<Event>(freeHandler);
  events::Subscription sub2 = events::subscribe<Event>(freeCountHandler);

  sub2 = std::move(sub1);
  ASSERT_FALSE(sub1.isConnected());
  ASSERT_TRUE(sub2.isConnected());

  events::publish(Event{5});
  QTest::qWait(1);
  ASSERT_EQ(g_freeValue, 5);
  ASSERT_EQ(g_freeCallCount, 0);

  sub2.reset();
  ASSERT_FALSE(sub2.isConnected());
}

TEST_F(EventTest, RepeatedResetIsSafe) {
  g_freeValue = 0;
  events::Subscription sub = events::subscribe<Event>(freeHandler);

  events::publish(Event{3});
  QTest::qWait(1);
  ASSERT_EQ(g_freeValue, 3);

  sub.reset();
  sub.reset();
  sub.reset();
  ASSERT_FALSE(sub.isConnected());

  g_freeValue = 0;
  events::publish(Event{4});
  QTest::qWait(1);
  ASSERT_EQ(g_freeValue, 0);
}

TEST_F(EventTest, DispatchersAreApplicationOwned) {
  // White-box test: inspects internal dispatcher QObject parenting via
  // findChildren — not public API usage. First use creates the dispatcher
  // via the public publish API. BusRegistry::dispatcher<T>() is private;
  // ownership is observable through the QObject parent tree —
  // dispatchers are parented to QCoreApplication.
  events::publish(Event{1});
  QTest::qWait(1);

  const auto children = QCoreApplication::instance()
                            ->findChildren<events::EventDispatcherBase *>();
  ASSERT_FALSE(children.isEmpty());
  for (const auto *child : children) {
    EXPECT_EQ(child->parent(), QCoreApplication::instance());
  }
}

TEST_F(EventTest, FreeFunctionSubscriptionUsesApplicationContext) {
  // Free-function subscriptions use QCoreApplication::instance() as the
  // connection context — no wrapper QObject is allocated. Delivery and
  // reset() (disconnect only, no deleteLater) must still work.
  g_freeValue = 0;
  events::Subscription sub = events::subscribe<Event>(freeHandler);
  ASSERT_TRUE(sub.isConnected());

  events::publish(Event{8});
  QTest::qWait(1);
  ASSERT_EQ(g_freeValue, 8);

  // Reset must not crash and must stop delivery even if called repeatedly
  // while the event loop is running (no deleteLater dependency).
  sub.reset();
  sub.reset();
  g_freeValue = 0;
  events::publish(Event{9});
  QTest::qWait(1);
  ASSERT_EQ(g_freeValue, 0);
}

TEST_F(EventTest, TypeMismatchDoesNotDeliverWrongData) {
  // Public API is typed (publish<T>); mismatches are not reachable without
  // internal QVariant abuse. Lock successful typed delivery and isolation
  // between different event types.
  int actual = 0;
  IntReceiver r(&actual);
  [[maybe_unused]] const auto sub =
      events::subscribe<Event>(&r, &IntReceiver::receive);
  events::publish(Event{11});
  QTest::qWait(1);
  ASSERT_EQ(actual, 11);

  int a = 0;
  int b = 0;
  MultiReceiver m(&a, &b);
  [[maybe_unused]] const auto subA =
      events::subscribe<EventA>(&m, &MultiReceiver::recvA);

  // Publishing Event must not deliver to EventA subscribers.
  events::publish(Event{5});
  QTest::qWait(1);
  EXPECT_EQ(a, 0);

  events::publish(EventA{99});
  QTest::qWait(1);
  EXPECT_EQ(a, 99);
}

// Wave 3: same-type recursive publish is queued, not stack-recursive.
class RecursiveSameTypePublisher : public QObject {
  Q_OBJECT
public:
  int *count;
  explicit RecursiveSameTypePublisher(int *c) : count(c) {}

public slots:
  void onEvent(const Event &e) {
    ++(*count);
    if (*count == 1) {
      events::publish(Event{e.value + 1});
    }
  }
};

TEST_F(EventTest, SameTypeRecursivePublishIsQueuedNotStackRecursive) {
  int count = 0;
  RecursiveSameTypePublisher receiver(&count);
  [[maybe_unused]] const auto sub = events::subscribe<Event>(
      &receiver, &RecursiveSameTypePublisher::onEvent);

  events::publish(Event{1});
  // Two event-loop iterations: original + recursive publish.
  QTest::qWait(50);

  EXPECT_EQ(count, 2)
      << "Same-type recursive publish should deliver exactly once more";
}

// Wave 3: type-mismatch branch via direct signal abuse (findChildren path).
// White-box test: invokeMethod on the internal dispatcher QObject — not
// public API usage. Typed public API cannot produce a mismatch; this locks
// defense-in-depth.
TEST_F(EventTest, TypeMismatchViaDirectSignalIsNotDelivered) {
  int received = 0;
  IntReceiver receiver(&received);
  [[maybe_unused]] const auto sub =
      events::subscribe<Event>(&receiver, &IntReceiver::receive);

  auto dispatchers =
      QCoreApplication::instance()->findChildren<events::EventDispatcherBase *>();
  ASSERT_FALSE(dispatchers.empty());

  const QVariant wrong = QVariant::fromValue(Event2{QStringLiteral("mismatch")});
  QMetaObject::invokeMethod(dispatchers.first(), "eventPublished",
                            Qt::DirectConnection, Q_ARG(QVariant, wrong));

  QTest::qWait(20);
  EXPECT_EQ(received, 0)
      << "Mismatched QVariant must not deliver to typed Event subscribers";
}

// Wave 2 (ES-005): reset() disconnects future delivery; already-posted queued
// events may still deliver to a still-alive receiver. Locks that contract.
TEST_F(EventTest, FreeFunctionResetPreventsFutureDeliveryAndAllowsInFlight) {
  g_freeValue = 0;
  g_freeCallCount = 0;

  events::Subscription sub = events::subscribe<Event>(freeCountHandler);

  // Publish without waiting — event is queued.
  events::publish(Event{1});
  // Immediately reset before the event loop runs.
  sub.reset();
  ASSERT_FALSE(sub.isConnected());

  QTest::qWait(5);

  // In-flight delivery is allowed (at most once for one publish). Per the
  // documented contract, events already posted to a still-alive receiver's
  // queue MAY still be delivered after reset().
  EXPECT_LE(g_freeCallCount, 1);

  const int afterInFlight = g_freeCallCount;

  // Future publishes must not deliver.
  events::publish(Event{2});
  QTest::qWait(5);

  EXPECT_EQ(g_freeCallCount, afterInFlight)
      << "reset() must prevent future delivery after the in-flight window";
}

// Wave 2 (ES-006): per-connection FIFO via Qt queued connections. Distinct
// payloads published rapidly must arrive in publish order.
TEST_F(EventTest, PerConnectionFifoPreservesDistinctPayloadOrder) {
  std::vector<int> received;
  ValueCollector collector(&received);
  [[maybe_unused]] const auto sub =
      events::subscribe<Event>(&collector, &ValueCollector::collect);

  constexpr int count = 10;
  for (int i = 1; i <= count; ++i) {
    events::publish(Event{i});
  }
  QTest::qWait(20);

  ASSERT_EQ(received.size(), static_cast<std::size_t>(count));
  for (int i = 0; i < count; ++i) {
    EXPECT_EQ(received[static_cast<std::size_t>(i)], i + 1)
        << "Per-connection FIFO must preserve distinct payload order at index "
        << i;
  }
}

// AUD-105: QObject subscribe returns a usable Subscription handle.
TEST_F(EventTest, QObjectSubscribeReturnsUsableHandle) {
  int actual = 0;
  IntReceiver r(&actual);
  events::Subscription sub = events::subscribe<Event>(&r, &IntReceiver::receive);
  ASSERT_TRUE(sub.isConnected());

  events::publish(Event{5});
  QTest::qWait(1);
  ASSERT_EQ(actual, 5);

  // Early unsubscribe via the retained handle still works for QObject
  // receivers (reset() disconnects even in receiver-lifetime mode).
  sub.reset();
  ASSERT_FALSE(sub.isConnected());
  events::publish(Event{6});
  QTest::qWait(1);
  ASSERT_EQ(actual, 5);
}

// AUD-105: discarding a QObject-receiver Subscription is the normal Qt
// pattern — connection lifetime follows the receiver. Unlike free-function
// handles, destruction of a discarded receiver-lifetime handle does not
// disconnect.
TEST_F(EventTest, DiscardedQObjectSubscriptionKeepsReceiverLifetimeDelivery) {
  int actual = 0;
  {
    IntReceiver r(&actual);
    [[maybe_unused]] const events::Subscription discarded =
        events::subscribe<Event>(&r, &IntReceiver::receive);

    events::publish(Event{7});
    QTest::qWait(1);
    ASSERT_EQ(actual, 7);
  } // receiver destroyed → Qt auto-disconnect
}

// AUD-120: cross-subscriber order follows Qt connection creation order.
// Contract documents this as an implementation/Qt-derived property; this
// test locks the observed [A, B] order for subscribe-A-then-B + one publish.
TEST_F(EventTest, CrossSubscriberOrderFollowsConnectionCreationOrder) {
  std::vector<int> order;
  OrderRecorder a(&order, 1);
  OrderRecorder b(&order, 2);

  [[maybe_unused]] const auto subA =
      events::subscribe<Event>(&a, &OrderRecorder::record);
  [[maybe_unused]] const auto subB =
      events::subscribe<Event>(&b, &OrderRecorder::record);

  events::publish(Event{0});
  QTest::qWait(20);

  ASSERT_EQ(order.size(), static_cast<std::size_t>(2));
  EXPECT_EQ(order[0], 1) << "First subscriber must run first (connection order)";
  EXPECT_EQ(order[1], 2) << "Second subscriber must run second";
}

// AUD-101: wrong-thread publish must be rejected (qCCritical + no-op) before
// any Q_ASSERT. Handler must not run. Captures qCCritical(app.event) via a
// test-local message-handler probe (ServiceRegistrationTest pattern).
TEST_F(EventTest, WrongThreadPublishIsRejectedWithoutDelivery) {
  g_wrongThreadHits.store(0, std::memory_order_relaxed);
  g_eventCriticals.store(0, std::memory_order_relaxed);

  events::Subscription sub = events::subscribe<Event>(wrongThreadHandler);
  ASSERT_TRUE(sub.isConnected());

  g_previousHandler = qInstallMessageHandler(eventCriticalProbe);

  std::thread worker([] { events::publish(Event{1}); });
  worker.join();

  qInstallMessageHandler(g_previousHandler);
  g_previousHandler = nullptr;

  QTest::qWait(20);

  EXPECT_EQ(g_wrongThreadHits.load(std::memory_order_relaxed), 0)
      << "wrong-thread publish must not deliver to handlers";
  EXPECT_GE(g_eventCriticals.load(std::memory_order_relaxed), 1)
      << "wrong-thread publish must log qCCritical(app.event)";
}

// AUD-101: wrong-thread free-function subscribe must be rejected
// (qCCritical + empty Subscription) before any Q_ASSERT.
TEST_F(EventTest, WrongThreadFreeFunctionSubscribeIsRejected) {
  g_wrongThreadHits.store(0, std::memory_order_relaxed);
  g_eventCriticals.store(0, std::memory_order_relaxed);

  g_previousHandler = qInstallMessageHandler(eventCriticalProbe);

  std::thread worker([] {
    // [[nodiscard]] free-fn subscribe: capture the (empty) handle.
    // Destruction of an empty Subscription is a no-op.
    events::Subscription late = events::subscribe<Event>(wrongThreadHandler);
    EXPECT_FALSE(late.isConnected())
        << "wrong-thread free-fn subscribe must return an empty handle";
  });
  worker.join();

  qInstallMessageHandler(g_previousHandler);
  g_previousHandler = nullptr;

  QTest::qWait(20);

  EXPECT_EQ(g_wrongThreadHits.load(std::memory_order_relaxed), 0);
  EXPECT_GE(g_eventCriticals.load(std::memory_order_relaxed), 1)
      << "wrong-thread free-fn subscribe must log qCCritical(app.event)";
}

// AUD-101: wrong-thread QObject subscribe must be rejected the same way.
TEST_F(EventTest, WrongThreadQObjectSubscribeIsRejected) {
  g_eventCriticals.store(0, std::memory_order_relaxed);

  g_previousHandler = qInstallMessageHandler(eventCriticalProbe);

  int actual = 0;
  std::thread worker([&actual] {
    IntReceiver r(&actual);
    events::Subscription late =
        events::subscribe<Event>(&r, &IntReceiver::receive);
    EXPECT_FALSE(late.isConnected())
        << "wrong-thread QObject subscribe must return an empty handle";
  });
  worker.join();

  qInstallMessageHandler(g_previousHandler);
  g_previousHandler = nullptr;

  EXPECT_GE(g_eventCriticals.load(std::memory_order_relaxed), 1)
      << "wrong-thread QObject subscribe must log qCCritical(app.event)";
}

// AUD-004 quit-lifecycle: parent test. True aboutToQuit permanently sets
// quitFired_ for this process's BusRegistry (single QApplication in
// tests/main.cpp). Firing it in-process would poison every later test in the
// UnitTests binary. Isolate the destructive scenario in a child process.
//
// Honest scope: this locks the real aboutToQuit signal path (not a
// simulation). The child runs QuitLifecycleAfterAboutToQuit below.
TEST_F(EventTest, QuitLifecycleGuardsLockBehaviorInSubprocess) {
  QProcess child;
  child.setProcessChannelMode(QProcess::ForwardedChannels);

  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  env.insert(QStringLiteral("EVENTSYSTEM_QUIT_SUBPROCESS"), QStringLiteral("1"));
  child.setProcessEnvironment(env);

  const QStringList args = {
      QStringLiteral("--gtest_filter=EventTest.QuitLifecycleAfterAboutToQuit"),
  };
  child.start(QCoreApplication::applicationFilePath(), args);
  ASSERT_TRUE(child.waitForStarted(15000));
  ASSERT_TRUE(child.waitForFinished(30000))
      << "quit-lifecycle subprocess timed out";
  EXPECT_EQ(child.exitCode(), 0)
      << "quit-lifecycle subprocess reported failure";
}

// AUD-004: destructive quit-lifecycle scenario. Runs in a subprocess when
// launched by QuitLifecycleGuardsLockBehaviorInSubprocess; skips in-process
// so the shared UnitTests QApplication + BusRegistry stay usable.
//
// SIMULATED vs REAL: this fires the REAL QCoreApplication::aboutToQuit
// signal via QMetaObject::invokeMethod — not a hand-rolled flag poke. What
// is NOT covered in-process: full QApplication destruction + a new instance
// (AUD-123 re-arm). That path requires multi-app which Qt forbids in one
// process; re-arm remains locked by contract + code inspection.
TEST_F(EventTest, QuitLifecycleAfterAboutToQuit) {
  if (qEnvironmentVariableIsEmpty("EVENTSYSTEM_QUIT_SUBPROCESS")) {
    GTEST_SKIP() << "destructive quit test; run via "
                    "QuitLifecycleGuardsLockBehaviorInSubprocess";
  }

  g_quitProbeHits = 0;

  // Subscription established BEFORE quit — must not receive post-quit events.
  events::Subscription sub = events::subscribe<Event>(quitProbeHandler);
  ASSERT_TRUE(sub.isConnected());

  // Baseline: delivery works before aboutToQuit.
  events::publish(Event{1});
  QTest::qWait(20);
  ASSERT_EQ(g_quitProbeHits, 1) << "pre-quit delivery must work";

  // Fire the real aboutToQuit signal (subprocess only).
  ASSERT_TRUE(QMetaObject::invokeMethod(QCoreApplication::instance(),
                                        "aboutToQuit",
                                        Qt::DirectConnection));

  // Post-quit publish must refuse (qCCritical + no-op) without crash.
  g_quitProbeHits = 0;
  events::publish(Event{2});
  QTest::qWait(20);
  EXPECT_EQ(g_quitProbeHits, 0)
      << "publish after aboutToQuit must not deliver";

  // Post-quit subscribe must refuse (empty handle) without crash.
  events::Subscription late = events::subscribe<Event>(quitProbeHandler);
  EXPECT_FALSE(late.isConnected())
      << "subscribe after aboutToQuit must return an empty handle";
  events::publish(Event{3});
  QTest::qWait(20);
  EXPECT_EQ(g_quitProbeHits, 0);

  // Map-clear safety: the aboutToQuit hook cleared the registry's non-owning
  // map while dispatcher QObjects still exist as app children (app is not
  // destroyed here). findChildren must not crash; no registry dereference
  // of cleared entries occurs on these public-API paths.
  const auto children = QCoreApplication::instance()
                            ->findChildren<events::EventDispatcherBase *>();
  Q_UNUSED(children);
  SUCCEED();
}

#include "EventsTest.moc"
// NOLINTEND
