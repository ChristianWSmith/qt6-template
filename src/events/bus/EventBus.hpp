
#pragma once

#include <any>
#include <atomic>
#include <cstdint>
#include <fmt/core.h>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <typeindex>
#include <unordered_map>
#include <utility>

namespace events {

template <typename T> class EventBus;

template <typename T> class Subscription;

namespace detail {
class BusRegistry {
public:
  template <typename T> static EventBus<T> &getBus() {
    const std::type_index type = std::type_index(typeid(T));
    std::unique_lock lock(instance().mutex_);

    auto busIterator = instance().buses_.find(type);
    if (busIterator != instance().buses_.end()) {

      return *std::any_cast<std::shared_ptr<EventBus<T>>>(busIterator->second);
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
Subscription<T> subscribe(std::function<void(const T &)> func) {
  return detail::BusRegistry::getBus<T>().subscribe(std::move(func));
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

  Subscription<T> subscribe(Callback callback) {
    std::unique_lock lock(mutex_);
    SubscriptionId subscriptionId = nextId_++;
    subscribers_[subscriptionId] = std::move(callback);
    return Subscription<T>(this, subscriptionId);
  }

  void unsubscribe(SubscriptionId subscriptionId) {
    std::unique_lock lock(mutex_);
    subscribers_.erase(subscriptionId);
  }

  void publish(const T &event) {
    {
      std::unique_lock lock(mutex_);
      last_ = event;
    }

    std::shared_lock lock(mutex_);
    for (const auto &[subscriptionId, func] : subscribers_) {
      std::thread(func, event).detach();
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
template <typename T> class Subscription {
public:
  Subscription(EventBus<T> *bus, uint64_t subscriptionId)
      : bus_(bus), id_(subscriptionId) {}

  Subscription(Subscription &&other) noexcept
      : bus_(std::exchange(other.bus_, nullptr)), id_(other.id_) {}

  Subscription &operator=(Subscription &&other) noexcept {
    if (this != &other) {
      unsubscribe();
      bus_ = std::exchange(other.bus_, nullptr);
      id_ = other.id_;
    }
    return *this;
  }

  ~Subscription() { unsubscribe(); }

  void unsubscribe() {
    if (bus_) {
      bus_->unsubscribe(id_);
      bus_ = nullptr;
    }
  }

  Subscription(const Subscription &) = delete;
  Subscription &operator=(const Subscription &) = delete;

private:
  EventBus<T> *bus_;
  uint64_t id_;
};

} // namespace events
