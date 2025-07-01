// NOLINTBEGIN
#include "events/system/EventSystem.hpp"
#include <QTest>
#include <chrono>
#include <condition_variable>
#include <mutex>

#include <gtest/gtest.h>

struct Event {
  int value;
};

class EventTest : public ::testing::Test {
protected:
  EventTest() {}
};

TEST_F(EventTest, EventsWork) {

  std::mutex mtx;
  std::condition_variable cv;
  bool event_received = false;

  int actual = 0;
  int expected = 1;

  events::subscribe<Event>([&](const Event &event) {
    {
      std::lock_guard<std::mutex> lock(mtx);
      actual = event.value;
      event_received = true;
    }
    cv.notify_one();
  });

  events::publish(Event{expected});
  QTest::qWait(1);

  std::unique_lock<std::mutex> lock(mtx);
  bool got_event = cv.wait_for(lock, std::chrono::seconds(1),
                               [&] { return event_received; });

  ASSERT_TRUE(got_event);
  ASSERT_EQ(actual, expected);
  events::publish(Event{expected});
}

// NOLINTEND
