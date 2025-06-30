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

// Base interface for all event buses
class IEventBus {
public:
  virtual ~IEventBus() = default;
  virtual void shutdown() = 0;
};

class BusRegistry {
public:
  template <typename T>
  static EventBus<T> &getBus() {
    const std::type_index type = std::type_index(typeid(T));
    std::unique_lock lock(instance().mutex_);

    auto it = instance().buses_.find(type);
    if (it != instance().buses_.end()) {
      return *static_cast<EventBus<T> *>(it->second.get());
    }

    auto newBus = std::make_shared<EventBus<T>>();
    instance().buses_[type] = newBus;
    return *newBus;
  }

  static void shutdown() {
    std::unique_lock lock(instance().mutex_);
    for (auto &[type, bus] : instance().buses_) {
      bus->shutdown();
    }
    instance().buses_.clear();
  }

private:
  static BusRegistry &instance() {
    static BusRegistry registry;
    return registry;
  }

  std::unordered_map<std::type_index, std::shared_ptr<IEventBus>> buses_;
  std::mutex mutex_;
};

template <typename T>
Subscription<T> subscribe(std::function<void(const T &)> func) {
  return BusRegistry::getBus<T>().subscribe(std::move(func));
}

template <typename T>
void publish(const T &event) {
  BusRegistry::getBus<T>().publish(event);
}

template <typename T>
T current() {
  return BusRegistry::getBus<T>().current();
}

template <typename T>
class EventBus : public IEventBus {
public:
  using Callback = std::function<void(const T &)>;
  using SubscriptionId = uint64_t;

  Subscription<T> subscribe(Callback callback) {
    std::unique_lock lock(mutex_);
    SubscriptionId subId = nextId_++;
    subscribers_[subId] = std::move(callback);
    return Subscription<T>(this, subId);
  }

  void unsubscribe(SubscriptionId subId) {
    std::unique_lock lock(mutex_);
    subscribers_.erase(subId);

    auto subIt = subscriberThreads_.find(subId);
    if (subIt != subscriberThreads_.end()) {
      for (std::thread &subThread : subIt->second) {
        if (subThread.joinable()) {
          subThread.join();
        }
      }
      subscriberThreads_.erase(subIt);
    }
  }

  void publish(const T &event) {
    std::shared_lock lock(mutex_);
    last_ = event;

    for (const auto &entry : subscribers_) {
      SubscriptionId subId = entry.first;
      const Callback &func = entry.second;

      std::thread subThread([func, event]() {
        try {
          func(event);
        } catch (const std::exception &e) {
          fmt::print(stderr, "Exception in EventBus callback: {}\n", e.what());
        } catch (...) {
          fmt::print(stderr, "Unknown exception in EventBus callback\n");
        }
      });

      subscriberThreads_[subId].emplace_back(std::move(subThread));
    }
  }

  T current() const {
    std::shared_lock lock(mutex_);
    return last_;
  }

  void shutdown() override {
    std::unique_lock lock(mutex_);

    for (auto &[subId, threads] : subscriberThreads_) {
      for (std::thread &subThread : threads) {
        if (subThread.joinable()) {
          subThread.join();
        }
      }
    }

    subscriberThreads_.clear();
    subscribers_.clear();
  }

private:
  mutable std::shared_mutex mutex_;
  std::unordered_map<SubscriptionId, Callback> subscribers_;
  std::unordered_map<SubscriptionId, std::vector<std::thread>> subscriberThreads_;
  std::atomic<SubscriptionId> nextId_ = 0;
  T last_;
};

template <typename T>
class Subscription {
public:
  Subscription(EventBus<T> *bus, uint64_t subId)
      : bus_(bus), id_(subId) {}

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
