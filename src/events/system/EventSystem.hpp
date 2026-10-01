#pragma once

#include <QCoreApplication>
#include <QObject>
#include <QThread>
#include <QVariant>
#include <concepts>
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
///   - Cross-subscriber order follows Qt connection creation order. This is
///     an implementation/Qt-derived property, not a strong application
///     guarantee unless explicitly tested.
///   - Mid-dispatch subscribe does not receive the in-flight event.
///   - Publish-during-dispatch is deferred to later event-loop iterations.
///   - Recursive publication is queued rather than immediate.
///
/// Lifetime
///   - QObject-receiver subscribe: connection lifetime follows the receiver.
///   - Free-function subscribe: caller must retain the Subscription
///   ([[nodiscard]]).
///   - Discarding a Subscription disconnects the handler immediately.
///   - Subscription::reset() disconnects logical delivery; it does not depend
///     on deleteLater() to unsubscribe and does not require a running event
///     loop.
///   - Free-function subscriptions belong to application lifetime (connection
///     context = QCoreApplication). The static Subscription in ServiceRegistry
///     is storage only; services::unregisterAll() is the explicit lifecycle
///     mechanism.
///
/// Ownership
///   - Dispatchers are parented to QCoreApplication when available
///     (application-owned lifetime).
///   - Free-function subscriptions use QCoreApplication::instance() as the
///     connection context; no wrapper QObject is allocated per subscription.
///   - Publish/subscribe require a running QApplication in this template.
///   - BusRegistry clears its non-owning dispatcher map on
///     QCoreApplication::aboutToQuit — before Qt-owned dispatcher QObjects
///     are destroyed. The registry never deletes dispatchers.
///
/// Threading
///   - Publish from the GUI thread only (template policy; no worker threads).
///   - The GUI-thread policy is enforced with Q_ASSERT in debug builds at
///     publish and both subscribe paths. Release builds keep the runtime
///     QCoreApplication::instance() checks only.
///   - Callbacks execute on the receiver QObject's thread via queued delivery.
///     Free-function handlers execute on the application thread under the
///     GUI-thread policy.
///   - BusRegistry mutex protects dispatcher creation, not event delivery.
///   - The bus is not a general cross-thread synchronization mechanism.
///
/// Service registration lifecycle
///   - QApplication exists → services::registerAll() → application runs →
///     services::unregisterAll() → QApplication destruction.
///   - unregisterAll() resets retained subscriptions explicitly; it is
///     idempotent and independent of static destructor timing.
///
/// API boundary
///   - events::publish and events::subscribe (free functions) are the
///     supported public-facing entry points. BusRegistry also exposes
///     equivalent static operations; consumers should use the free-function
///     facade.
///   - BusRegistry::dispatcher<T>() is an internal implementation detail
///     (private). Do not rely on direct dispatcher access.
///   - Runtime QVariant type check remains as defense-in-depth against
///     QVariant corruption; the primary protection is the typed public API
///     plus the private dispatcher boundary.
///
/// Type safety
///   - Event types must be default-constructible, copy-constructible, and
///     copy-assignable (compile-time; EventType concept).
///   - Delivery checks QVariant meta-type in all builds; debug builds also
///     assert. Mismatch logs qCritical(appEvent) and does not deliver.

template <typename T>
concept EventType =
    std::default_initializable<T> && std::copy_constructible<T> &&
    std::assignable_from<T &, const T &>;

namespace detail {

template <typename T> bool checkedEventValue(const QVariant &var, T &out) {
  if (var.userType() != qMetaTypeId<T>()) {
    qCCritical(appEvent)
        << "EventSystem: type mismatch; event not delivered. expected"
        << QMetaType(qMetaTypeId<T>()).name() << "got"
        << QMetaType(var.userType()).name();
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
    if (!QCoreApplication::instance()) {
      qCCritical(appEvent)
          << "EventSystem: publish requires a running QCoreApplication";
      return;
    }
    Q_ASSERT(QThread::currentThread() ==
             QCoreApplication::instance()->thread());
    emit eventPublished(QVariant::fromValue(event));
  }
};

/// RAII free-function subscription handle. The returned Subscription must be
/// retained; discarding it disconnects the handler immediately.
class Subscription {
public:
  Subscription() noexcept = default;
  ~Subscription() { reset(); }

  Subscription(const Subscription &) = delete;
  Subscription &operator=(const Subscription &) = delete;

  Subscription(Subscription &&other) noexcept
      : connection_(std::exchange(other.connection_, {})) {}

  Subscription &operator=(Subscription &&other) noexcept {
    if (this != &other) {
      reset();
      connection_ = std::exchange(other.connection_, {});
    }
    return *this;
  }

  [[nodiscard]] bool isConnected() const {
    return static_cast<bool>(connection_);
  }

  /// Logical unsubscription: disconnect only. The connection context is
  /// application-owned (QCoreApplication). reset() does not call
  /// deleteLater() and does not require a running event loop.
  void reset() {
    if (static_cast<bool>(connection_)) {
      QObject::disconnect(connection_);
      connection_ = {};
    }
  }

  explicit Subscription(QMetaObject::Connection conn)
      : connection_(std::move(conn)) {}

private:
  QMetaObject::Connection connection_;
};

/// Internal event bus. Public API is events::publish / events::subscribe
/// (thin wrappers over the static methods below). dispatcher<T>() is a
/// private implementation detail — not a public mutable-ref escape hatch.
class BusRegistry {
public:
  template <EventType T> static void publish(T event) {
    // Pre-check before touching the dispatcher map: after application
    // teardown the map is cleared on aboutToQuit, and a late publish must
    // not construct an unparented dispatcher or bind a stale reference.
    if (!QCoreApplication::instance()) {
      qCCritical(appEvent)
          << "EventSystem: publish requires a running QCoreApplication";
      return;
    }
    Q_ASSERT(QThread::currentThread() ==
             QCoreApplication::instance()->thread());
    dispatcher<T>().publish(event);
  }

  /// QObject-receiver subscribe. Connection lifetime follows the receiver.
  template <EventType T, typename Obj>
  static void subscribe(Obj *receiver, void (Obj::*method)(const T &))
    requires(std::is_base_of_v<QObject, Obj>)
  {
    if (!QCoreApplication::instance()) {
      qCCritical(appEvent)
          << "EventSystem: QObject subscribe requires a running"
          << "QCoreApplication";
      return;
    }
    Q_ASSERT(QThread::currentThread() ==
             QCoreApplication::instance()->thread());
    QObject::connect(
        &dispatcher<T>(), &EventDispatcherBase::eventPublished, receiver,
        [receiver, method](const QVariant &var) {
          T event{};
          if (!detail::checkedEventValue(var, event)) {
            return;
          }
          (receiver->*method)(event);
        },
        Qt::QueuedConnection);
  }

  /// Free-function subscribe. Uses QCoreApplication::instance() as the
  /// connection context — no wrapper QObject is allocated. Free-function
  /// handlers execute on the application thread under the GUI-thread policy.
  /// Subscriptions belong to application lifetime; services::unregisterAll()
  /// is the explicit teardown mechanism for retained service subscriptions.
  template <EventType T>
  [[nodiscard]] static Subscription subscribe(void (*func)(const T &)) {
    QObject *context = QCoreApplication::instance();
    if (!context) {
      qCCritical(appEvent)
          << "EventSystem: free-function subscribe requires a running"
          << "QCoreApplication";
      return Subscription{};
    }
    Q_ASSERT(QThread::currentThread() == context->thread());
    auto conn = QObject::connect(
        &dispatcher<T>(), &EventDispatcherBase::eventPublished, context,
        [func](const QVariant &var) {
          T event{};
          if (!detail::checkedEventValue(var, event)) {
            return;
          }
          func(event);
        },
        Qt::QueuedConnection);
    return Subscription(std::move(conn));
  }

private:
  /// Internal implementation detail. Do not expose; use the free-function
  /// events::publish / events::subscribe facade.
  template <EventType T> static EventDispatcher<T> &dispatcher() {
    const std::type_index type = typeid(T);
    BusRegistry &reg = instance();
    std::unique_lock lock(reg.mutex_);

    // Structural lifetime guarantee (AUD-001):
    //   On first dispatcher creation, connect QCoreApplication::aboutToQuit
    //   to clear the non-owning map while QObject children (dispatchers)
    //   still exist. Qt then destroys the dispatcher objects with the
    //   application. The map never deletes or dereferences entries after
    //   that clear; adding a BusRegistry destructor that iterates+deletes
    //   would double-delete against Qt ownership.
    if (!reg.quitHookInstalled_) {
      if (auto *app = QCoreApplication::instance()) {
        QObject::connect(
            app, &QCoreApplication::aboutToQuit, app, []() {
              BusRegistry &r = instance();
              std::unique_lock clearLock(r.mutex_);
              r.dispatchers_.clear();
            });
        reg.quitHookInstalled_ = true;
      }
    }

    // Non-owning: QObject parent (QCoreApplication) owns the dispatcher when
    // one exists. Raw pointer avoids double-delete against Qt parent/child.
    EventDispatcherBase *&basePtr = reg.dispatchers_[type];
    if (!basePtr) {
      // Parented to QCoreApplication when present (application-owned).
      // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
      auto *typed = new EventDispatcher<T>(QCoreApplication::instance());
      basePtr = typed;
      qRegisterMetaType<T>();
    }

    return *static_cast<EventDispatcher<T> *>(basePtr);
  }

  static BusRegistry &instance() {
    static BusRegistry busRegistry;
    return busRegistry;
  }

  std::unordered_map<std::type_index, EventDispatcherBase *> dispatchers_;
  std::mutex mutex_;
  bool quitHookInstalled_ = false;
};

/// QObject-receiver subscription. Thin wrapper over BusRegistry::subscribe.
template <EventType T, typename Obj>
void subscribe(Obj *receiver, void (Obj::*method)(const T &))
  requires(std::is_base_of_v<QObject, Obj>)
{
  BusRegistry::subscribe<T, Obj>(receiver, method);
}

/// Free-function subscription. The returned Subscription must be retained;
/// discarding it disconnects the handler immediately.
///
/// Uses QCoreApplication::instance() as the connection context — no wrapper
/// QObject is allocated. Free-function handlers execute on the application
/// thread under the GUI-thread policy. Subscriptions belong to application
/// lifetime; services::unregisterAll() is the explicit teardown mechanism for
/// retained service subscriptions.
///
/// Thin wrapper over BusRegistry::subscribe.
template <EventType T>
[[nodiscard]] Subscription subscribe(void (*func)(const T &)) {
  return BusRegistry::subscribe<T>(func);
}

/// Thin wrapper over BusRegistry::publish.
template <EventType T> void publish(T event) {
  BusRegistry::publish<T>(std::move(event));
}

} // namespace events
