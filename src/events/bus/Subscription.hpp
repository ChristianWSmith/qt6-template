
#pragma once

#include <cstdint>
#include <utility>

namespace events {

template <typename T> class EventBus;

template <typename T> class Subscription {
public:
  Subscription(EventBus<T> *bus, uint64_t id) : bus_(bus), id_(id) {}

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
