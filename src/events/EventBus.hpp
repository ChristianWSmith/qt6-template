
#pragma once

#include "Subscription.hpp"
#include <any>
#include <functional>
#include <memory>
#include <mutex>
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

} // namespace events
