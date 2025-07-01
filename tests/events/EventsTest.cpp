// NOLINTBEGIN
#include "events/system/EventSystem.hpp"
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

  events::subscribe<Event>([&](const Event &event) { actual = event.value; });

  events::publish(Event{expected});
  QTest::qWait(1);

  ASSERT_EQ(actual, expected);
  // events::publish(Event{expected}); // un-commenting this line leads to
  // subsequent tests hanging indefinitely
}

// NOLINTEND
