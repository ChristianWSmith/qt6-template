// NOLINTBEGIN
#include "events/system/EventSystem.hpp"
#include <QObject>
#include <QTest>
#include <gtest/gtest.h>

struct Event {
  int value;
};

class EventTest : public ::testing::Test {
protected:
  EventTest() {}
};

TEST_F(EventTest, EventsWork) {
  int actual = 0;
  int expected = 1;

  // Own the subscriber on the stack, so it's destroyed at scope exit:
  QObject qObj;
  events::subscribe<Event>(&qObj,
                           [&](const Event &event) { actual = event.value; });

  events::publish(Event{expected});
  QTest::qWait(100);

  ASSERT_EQ(actual, expected);
  events::publish(Event{expected});
}

TEST_F(EventTest, DestroyedSubscriberDoesNotReceiveEvents) {
  int actual = 0;

  {
    QObject qObj;
    events::subscribe<Event>(&qObj,
                             [&](const Event &event) { actual = event.value; });
    // qObj goes out of scope & is destroyed here, auto-disconnecting
  }

  events::publish(Event{123});
  QTest::qWait(100);

  ASSERT_EQ(actual, 0);
}

TEST_F(EventTest, MultipleSubscribersReceiveEvents) {
  int a = 0, b = 0;

  QObject objA, objB;
  events::subscribe<Event>(&objA, [&](const Event &event) { a = event.value; });
  events::subscribe<Event>(&objB, [&](const Event &event) { b = event.value; });

  events::publish(Event{42});
  QTest::qWait(100);

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
  QTest::qWait(100);

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

  QTest::qWait(100);
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
  QTest::qWait(100);

  ASSERT_EQ(actual, 0);
}

#include "EventsTest.moc"
// NOLINTEND
