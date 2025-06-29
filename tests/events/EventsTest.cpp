// NOLINTBEGIN
#include "events/bus/EventBus.hpp"
#include <chrono>
#include <condition_variable>
#include <mutex>

#include <gtest/gtest.h>

class EventTest : public ::testing::Test {
protected:
  EventTest() {}
};

TEST_F(EventTest, EventsWork) {
  struct Event {
    int value;
  };

  std::mutex mtx;
  std::condition_variable cv;
  bool event_received = false;

  int actual = 0;
  int expected = 1;

  auto sub = events::subscribe<Event>([&](const Event &event) {
    {
      std::lock_guard<std::mutex> lock(mtx);
      actual = event.value;
      event_received = true;
    }
    cv.notify_one();
  });

  events::publish(Event{expected});

  std::unique_lock<std::mutex> lock(mtx);
  bool got_event = cv.wait_for(lock, std::chrono::seconds(1),
                               [&] { return event_received; });

  ASSERT_TRUE(got_event);
  ASSERT_EQ(actual, expected);

  actual = events::current<Event>().value;
  ASSERT_EQ(actual, expected);
}

// NOLINTEND
