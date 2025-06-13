
#pragma once

#include "Subscription.hpp"
#include <any>
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <typeindex>
#include <unordered_map>
#include <utility>

#include "Subscription.hpp"

namespace events {

template <typename T> class EventBus;

template <typename T> class Subscription;

namespace detail {
class BusRegistry {
public:
  template <typename T> static EventBus<T> &getBus() {
    const std::type_index type = std::type_index(typeid(T));

    std::unique_lock lock(instance().mutex_);
    auto it = instance().buses_.find(type);
    if (it != instance().buses_.end()) {
      return *std::any_cast<std::shared_ptr<EventBus<T>>>(it->second);
    }

    auto newBus = std::make_shared<EventBus<T>>();
    instance().buses_[type] = newBus;
    return *newBus;
  }

private:
  static BusRegistry &instance() {
    static BusRegistry registry;
    return registry;
  }

  std::unordered_map<std::type_index, std::any> buses_;
  std::mutex mutex_;
};
} // namespace detail

template <typename T>
Subscription<T> subscribe(std::function<void(const T &)> fn) {
  return detail::BusRegistry::getBus<T>().subscribe(std::move(fn));
}

template <typename T> void publish(const T &event) {
  detail::BusRegistry::getBus<T>().publish(event);
}

template <typename T> T current() {
  return detail::BusRegistry::getBus<T>().current();
}

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
