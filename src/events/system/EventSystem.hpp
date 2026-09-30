#pragma once

#include <QCoreApplication>
#include <QObject>
#include <QVariant>
#include <mutex>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>

#include "../../logging/logging.h"

namespace events {

/// EventSystem contract (authoritative; also documented in AGENTS.md):
///
/// Delivery
///   - Always queued (Qt::QueuedConnection). Publish never invokes handlers
///     synchronously on the publisher's stack.
///   - Destroyed subscribers do not receive queued events targeted at them.
///
/// Ordering
///   - Per-connection FIFO via Qt queued connections.
///   - Cross-subscriber order follows connection creation order (Qt).
///   - Mid-dispatch subscribe does not receive the in-flight event.
///   - Publish-during-dispatch is deferred to later event-loop iterations.
///
/// Lifetime
///   - QObject-receiver subscribe: connection lifetime follows the receiver.
///   - Free-function subscribe: caller must retain the Subscription ([[nodiscard]]).
///   - Discarding a Subscription disconnects the handler immediately.
///   - Subscription::reset() disconnects logical delivery; it does not depend
///     on deleteLater() to unsubscribe.
///
/// Ownership
///   - Dispatchers and free-function wrapper QObjects are parented to
///     QCoreApplication when available (application-owned lifetime).
///   - Publish/subscribe require a running QApplication in this template.
///
/// Threading
///   - Publish from the GUI thread only (template policy; no worker threads).
///   - Callbacks execute on the receiver QObject's thread via queued delivery.
///   - BusRegistry mutex protects dispatcher creation, not event delivery.
///
/// Type safety
///   - Event types must be copy-constructible (compile-time).
///   - Delivery checks QVariant meta-type in all builds; debug builds also
///     assert. Mismatch logs qCritical(appEvent) and does not deliver.

namespace detail {

template <typename T>
bool checkedEventValue(const QVariant &var, T &out) {
  if (var.userType() != qMetaTypeId<T>()) {
    qCCritical(appEvent)
        << "EventSystem: type mismatch; event not delivered. expected"
        << QMetaType::typeName(qMetaTypeId<T>()) << "got"
        << QMetaType::typeName(var.userType());
    return false;
  }
  Q_ASSERT(var.userType() == qMetaTypeId<T>());
  out = var.value<T>();
  return true;
}

} // namespace detail

class EventDispatcherBase : public QObject {
  Q_OBJECT
public:
  explicit EventDispatcherBase(QObject *parent = nullptr) : QObject(parent) {}
  virtual ~EventDispatcherBase() = default;

  EventDispatcherBase(const EventDispatcherBase &) = delete;
  EventDispatcherBase &operator=(const EventDispatcherBase &) = delete;
  EventDispatcherBase(EventDispatcherBase &&) = delete;
  EventDispatcherBase &operator=(EventDispatcherBase &&) = delete;

signals:
  void eventPublished(const QVariant &event);
};

template <typename T> class EventDispatcher : public EventDispatcherBase {
public:
  using EventDispatcherBase::EventDispatcherBase;

  void publish(const T &event) {
    emit eventPublished(QVariant::fromValue(event));
  }
};

class BusRegistry {
public:
  template <typename T> static EventDispatcher<T> &dispatcher() {
    static_assert(std::is_copy_constructible_v<T>, "Events must be copyable");

    const std::type_index type = typeid(T);
    std::unique_lock lock(instance().mutex_);

    // Non-owning: QObject parent (QApplication) owns the dispatcher when one
    // exists. Raw pointer avoids double-delete against Qt parent/child.
    EventDispatcherBase *&basePtr = instance().dispatchers_[type];
    if (!basePtr) {
      auto *typed = new EventDispatcher<T>(QCoreApplication::instance());
      basePtr = typed;
      qRegisterMetaType<T>();
    }

    return *static_cast<EventDispatcher<T> *>(basePtr);
  }

  template <typename T> static void publish(T event) {
    dispatcher<T>().publish(event);
  }

private:
  static BusRegistry &instance() {
    static BusRegistry busRegistry;
    return busRegistry;
  }

  std::unordered_map<std::type_index, EventDispatcherBase *> dispatchers_;
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
        T event{};
        if (!detail::checkedEventValue(var, event)) {
          return;
        }
        (receiver->*method)(event);
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

  /// Logical unsubscription: disconnect only.
  /// The wrapper QObject is application-owned (parented to QCoreApplication
  /// when present) and is reclaimed with the application. reset() does not
  /// call deleteLater() and does not require a running event loop.
  void reset() {
    if (static_cast<bool>(connection_)) {
      QObject::disconnect(connection_);
      connection_ = {};
    }
    owner_ = nullptr;
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
  auto *wrapper = new QObject(QCoreApplication::instance());
  auto conn = QObject::connect(
      &BusRegistry::dispatcher<T>(), &EventDispatcherBase::eventPublished,
      wrapper,
      [func](const QVariant &var) {
        T event{};
        if (!detail::checkedEventValue(var, event)) {
          return;
        }
        func(event);
      },
      Qt::QueuedConnection);
  return Subscription(std::move(conn), wrapper);
}

template <typename T> void publish(T event) {
  BusRegistry::publish<T>(std::move(event));
}

} // namespace events
