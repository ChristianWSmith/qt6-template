
#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <unordered_map>

#include "Subscription.hpp"

namespace events {

template <typename T> class EventBus {
public:
  using Callback = std::function<void(const T &)>;
  using SubscriptionId = uint64_t;

  Subscription<T> subscribe(Callback cb) {
    std::unique_lock lock(mutex_);
    SubscriptionId id = nextId_++;
    subscribers_[id] = std::move(cb);
    return Subscription<T>(this, id);
  }

  void unsubscribe(SubscriptionId id) {
    std::unique_lock lock(mutex_);
    subscribers_.erase(id);
  }

  void publish(const T &event) {
    {
      std::unique_lock lock(mutex_);
      last_ = event;
    }

    std::shared_lock lock(mutex_);
    for (const auto &[_, fn] : subscribers_) {
      std::thread(fn, event).detach();
    }
  }

  T current() const {
    std::shared_lock lock(mutex_);
    return last_;
  }

private:
  mutable std::shared_mutex mutex_;
  std::unordered_map<SubscriptionId, Callback> subscribers_;
  std::atomic<SubscriptionId> nextId_ = 0;
  T last_;
};

} // namespace events
