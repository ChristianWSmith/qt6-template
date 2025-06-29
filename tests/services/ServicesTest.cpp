
// NOLINTBEGIN
#include "services/registry/ServiceRegistry.hpp"
#include <chrono>
#include <condition_variable>
#include <mutex>

#include <gtest/gtest.h>

class ServicesTest : public ::testing::Test {
protected:
  ServicesTest() {}
};

struct InputEvent {
  int value;
};
struct OutputEvent {
  int value;
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

  auto sub = events::subscribe<OutputEvent>([&](const OutputEvent &event) {
    {
      std::lock_guard<std::mutex> lock(mtx);
      actual = event.value;
      event_received = true;
    }
    cv.notify_one();
  });

  events::publish(InputEvent{expected});

  std::unique_lock<std::mutex> lock(mtx);
  bool got_event = cv.wait_for(lock, std::chrono::seconds(1),
                               [&] { return event_received; });

  ASSERT_TRUE(got_event);
  ASSERT_EQ(actual, expected);
}

// NOLINTEND
