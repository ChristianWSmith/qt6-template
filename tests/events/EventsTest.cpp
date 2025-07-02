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

class EventTest : public ::testing::Test {
protected:
  EventTest() {}
};

TEST_F(EventTest, EventsWork) {
  int actual = 0;
  int expected = 1;

  QObject qObj;
  events::subscribe<Event>(&qObj,
                           [&](const Event &event) { actual = event.value; });

  events::publish(Event{expected});
  QTest::qWait(1);

  ASSERT_EQ(actual, expected);
}

TEST_F(EventTest, DestroyedSubscriberDoesNotReceiveEvents) {
  int actual = 0;

  {
    QObject qObj;
    events::subscribe<Event>(&qObj,
                             [&](const Event &event) { actual = event.value; });
  }

  events::publish(Event{123});
  QTest::qWait(1);

  ASSERT_EQ(actual, 0);
}

TEST_F(EventTest, MultipleSubscribersReceiveEvents) {
  int a = 0, b = 0;

  QObject objA, objB;
  events::subscribe<Event>(&objA, [&](const Event &event) { a = event.value; });
  events::subscribe<Event>(&objB, [&](const Event &event) { b = event.value; });

  events::publish(Event{42});
  QTest::qWait(1);

  ASSERT_EQ(a, 42);
  ASSERT_EQ(b, 42);
}

TEST_F(EventTest, SubscriberCanPublishAnotherEvent) {
  struct Event2 {
    QString message;
  };

  QString actualMessage;
  QObject objA, objB;

  events::subscribe<Event>(&objA, [&](const Event &event) {
    events::publish(Event2{QString("Value was %1").arg(event.value)});
  });

  events::subscribe<Event2>(
      &objB, [&](const Event2 &event) { actualMessage = event.message; });

  events::publish(Event{7});
  QTest::qWait(1);

  ASSERT_EQ(actualMessage, "Value was 7");
}

TEST_F(EventTest, RapidFireEventsAllHandled) {
  constexpr int count = 100;
  int sum = 0;

  QObject obj;
  events::subscribe<Event>(&obj,
                           [&](const Event &event) { sum += event.value; });

  for (int i = 0; i < count; ++i) {
    events::publish(Event{1});
  }

  QTest::qWait(1);
  ASSERT_EQ(sum, count);
}

TEST_F(EventTest, LambdaLifetimeTiedToOwner) {
  int actual = 0;

  {
    QObject owner;
    events::subscribe<Event>(&owner,
                             [&](const Event &event) { actual = event.value; });
  }

  events::publish(Event{999});
  QTest::qWait(1);

  ASSERT_EQ(actual, 0);
}

TEST_F(EventTest, SubscriberCanRegisterAnotherSubscriberMidDispatch) {
  int a = 0;
  int b = 0;

  QObject objA, objB;

  events::subscribe<Event>(&objA, [&](const Event &) {
    a = 1;
    events::subscribe<Event>(&objB, [&](const Event &) { b = 1; });
  });

  events::publish(Event{});
  QTest::qWait(1);

  ASSERT_EQ(a, 1);
  ASSERT_EQ(b, 0);

  events::publish(Event{});
  QTest::qWait(1);

  ASSERT_EQ(b, 1);
}

TEST_F(EventTest, SubscriberCanUnsubscribeItselfSafely) {
  int callCount = 0;

  QPointer<QObject> obj = new QObject();
  events::subscribe<Event>(obj, [&](const Event &) {
    callCount++;
    obj->deleteLater();
  });

  events::publish(Event{});
  QTest::qWait(1);

  ASSERT_TRUE(obj.isNull());

  events::publish(Event{});
  QTest::qWait(1);

  ASSERT_EQ(callCount, 1);
}

TEST_F(EventTest, ThousandsOfSubscribersAllFire) {
  static constexpr int count = 1000;
  int hitCount = 0;

  std::vector<std::unique_ptr<QObject>> subs;
  subs.reserve(count);

  for (int i = 0; i < count; ++i) {
    auto obj = std::make_unique<QObject>();
    events::subscribe<Event>(obj.get(), [&](const Event &) { ++hitCount; });
    subs.push_back(std::move(obj));
  }

  events::publish(Event{});
  QTest::qWait(1);

  ASSERT_EQ(hitCount, count);
}

TEST_F(EventTest, OneObjectCanSubscribeToMultipleEvents) {
  int a = 0, b = 0;
  QObject shared;

  events::subscribe<EventA>(&shared, [&](const EventA &e) { a = e.val; });
  events::subscribe<EventB>(&shared, [&](const EventB &e) { b = e.val; });

  events::publish(EventA{1});
  events::publish(EventB{2});
  QTest::qWait(1);

  ASSERT_EQ(a, 1);
  ASSERT_EQ(b, 2);
}

#include "EventsTest.moc"

// NOLINTEND
