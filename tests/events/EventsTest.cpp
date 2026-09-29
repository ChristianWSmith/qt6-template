// NOLINTBEGIN
#include "events/system/EventSystem.hpp"
#include <QObject>
#include <QPointer>
#include <QTest>
#include <gtest/gtest.h>

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

// --- Free-function helpers ---

static int g_freeValue = 0;

static void freeHandler(const Event &e) { g_freeValue = e.value; }

static int g_freeCallCount = 0;

static void freeCountHandler(const Event &) { ++g_freeCallCount; }

// --- TESTS ---

TEST_F(EventTest, EventsWork) {
  int actual = 0;
  int expected = 1;

  IntReceiver r(&actual);
  events::subscribe<Event>(&r, &IntReceiver::receive);

  events::publish(Event{expected});
  QTest::qWait(1);

  ASSERT_EQ(actual, expected);
}

TEST_F(EventTest, DestroyedSubscriberDoesNotReceiveEvents) {
  int actual = 0;

  {
    IntReceiver r(&actual);
    events::subscribe<Event>(&r, &IntReceiver::receive);
  }

  events::publish(Event{123});
  QTest::qWait(1);

  ASSERT_EQ(actual, 0);
}

TEST_F(EventTest, MultipleSubscribersReceiveEvents) {
  int a = 0, b = 0;
  IntReceiver ra(&a), rb(&b);

  events::subscribe<Event>(&ra, &IntReceiver::receive);
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

  events::subscribe<Event>(&bridge, &Bridge::forward);
  events::subscribe<Event2>(&recv, &Event2Receiver::receive);

  events::publish(Event{7});
  QTest::qWait(10);

  ASSERT_EQ(actualMessage, "Value was 7");
}

TEST_F(EventTest, RapidFireEventsAllHandled) {
  constexpr int count = 100;
  int sum = 0;

  SummingReceiver receiver(&sum);
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

  events::subscribe<EventA>(&m, &MultiReceiver::recvA);
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

#include "EventsTest.moc"
// NOLINTEND
