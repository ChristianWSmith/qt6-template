#pragma once

#include <QObject>
#include <QPointer>
#include <QVariant>
#include <functional>
#include <memory>
#include <mutex>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>

namespace events {

class EventDispatcherBase : public QObject {
  Q_OBJECT
public:
  explicit EventDispatcherBase() = default;
  virtual ~EventDispatcherBase() = default;

  EventDispatcherBase(const EventDispatcherBase &) = delete;
  EventDispatcherBase &operator=(const EventDispatcherBase &) = delete;
  EventDispatcherBase(EventDispatcherBase &&) = delete;
  EventDispatcherBase &operator=(EventDispatcherBase &&) = delete;

  virtual void publish(const QVariant &event) = 0;

signals:
  void eventPublished(const QVariant &event);
};

template <typename T> class EventDispatcher : public EventDispatcherBase {
public:
  void publish(const T &event) {
    emit eventPublished(QVariant::fromValue(event));
  }

  void publish(const QVariant &event) override { emit eventPublished(event); }
};

class BusRegistry {
public:
  template <typename T> static EventDispatcher<T> &dispatcher() {
    static_assert(std::is_copy_constructible_v<T>, "Events must be copyable");

    const std::type_index type = typeid(T);
    std::unique_lock lock(instance().mutex_);

    auto &basePtr = instance().dispatchers_[type];
    if (!basePtr) {
      // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
      auto *typed = new EventDispatcher<T>();
      basePtr.reset(typed);
      qRegisterMetaType<T>();
    }

    return *static_cast<EventDispatcher<T> *>(basePtr.get());
  }

  template <typename T> static void publish(T event) {
    dispatcher<T>().publish(event);
  }

private:
  static BusRegistry &instance() {
    static BusRegistry busRegistry;
    return busRegistry;
  }

  std::unordered_map<std::type_index, std::unique_ptr<EventDispatcherBase>>
      dispatchers_;
  std::mutex mutex_;
};

template <typename T, typename Obj>
void subscribe(Obj *receiver, void (Obj::*method)(const T &))
  requires(std::is_base_of_v<QObject, Obj>)
{
  QObject::connect(
      &BusRegistry::dispatcher<T>(), &EventDispatcherBase::eventPublished,
      receiver,
      [receiver, method](const QVariant &var) {
        Q_ASSERT(var.userType() == qMetaTypeId<T>());
        (receiver->*method)(var.value<T>());
      },
      Qt::QueuedConnection);
}

class Subscription {
public:
  Subscription() = default;
  ~Subscription() { reset(); }

  Subscription(const Subscription &) = delete;
  Subscription &operator=(const Subscription &) = delete;

  Subscription(Subscription &&other) noexcept
      : connection_(std::exchange(other.connection_, {}))
      , owner_(std::exchange(other.owner_, nullptr)) {}

  Subscription &operator=(Subscription &&other) noexcept {
    if (this != &other) {
      reset();
      connection_ = std::exchange(other.connection_, {});
      owner_ = std::exchange(other.owner_, nullptr);
    }
    return *this;
  }

  [[nodiscard]] bool isConnected() const { return static_cast<bool>(connection_); }

  void reset() {
    if (static_cast<bool>(connection_)) {
      QObject::disconnect(connection_);
      connection_ = {};
    }
    if (owner_) {
      owner_->deleteLater();
      owner_ = nullptr;
    }
  }

  Subscription(QMetaObject::Connection conn, QObject *owner)
      : connection_(std::move(conn)), owner_(owner) {}

private:
  QMetaObject::Connection connection_;
  QObject *owner_ = nullptr;
};

/// Free-function subscription. The returned Subscription must be retained;
/// discarding it disconnects the handler immediately.
template <typename T>
[[nodiscard]] Subscription subscribe(void (*func)(const T &)) {
  auto *wrapper = new QObject();
  auto conn = QObject::connect(
      &BusRegistry::dispatcher<T>(), &EventDispatcherBase::eventPublished,
      wrapper,
      [func](const QVariant &var) {
        Q_ASSERT(var.userType() == qMetaTypeId<T>());
        func(var.value<T>());
      },
      Qt::QueuedConnection);
  return Subscription(std::move(conn), wrapper);
}

template <typename T> void publish(T event) {
  BusRegistry::publish<T>(std::move(event));
}

} // namespace events
