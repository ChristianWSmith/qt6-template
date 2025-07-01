
// NOLINTBEGIN
#include "services/registry/ServiceRegistry.hpp"
#include <QTest>
#include <chrono>
#include <condition_variable>
#include <mutex>

#include <gtest/gtest.h>

struct InputEvent {
  int value;
};

struct OutputEvent {
  int value;
};

class ServicesTest : public ::testing::Test {
protected:
  ServicesTest() {}
};

OutputEvent handle(InputEvent inputEvent) {
  return OutputEvent{inputEvent.value};
};

TEST_F(ServicesTest, ServicesWork) {
  services::ServiceRegistry::registerService<InputEvent, OutputEvent>(handle);

  std::mutex mtx;
  std::condition_variable cv;
  bool event_received = false;

  int actual = 0;
  int expected = 1;

  events::subscribe<OutputEvent>([&](const OutputEvent &event) {
    {
      std::lock_guard<std::mutex> lock(mtx);
      actual = event.value;
      event_received = true;
    }
    cv.notify_one();
  });

  events::publish(InputEvent{expected});
  QTest::qWait(1);

  std::unique_lock<std::mutex> lock(mtx);
  bool got_event = cv.wait_for(lock, std::chrono::seconds(1),
                               [&] { return event_received; });

  ASSERT_TRUE(got_event);
  ASSERT_EQ(actual, expected);
}

// NOLINTEND
