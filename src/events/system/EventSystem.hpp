#pragma once

#include <QCoreApplication>
#include <QObject>
#include <QPointer>
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
///   - QObject-receiver subscribe returns a Subscription. Connection
///     lifetime follows the receiver QObject (Qt auto-disconnect on receiver
///     destruction). Discarding that handle is the normal Qt pattern for a
///     receiver-lifetime connection and does not disconnect; reset() still
///     disconnects for early unsubscribe. The free-function facade marks the
///     QObject overload [[nodiscard]] for consistency — use a named variable
///     or (void) when intentionally discarding.
///   - Free-function subscribe: caller must retain the Subscription
///     ([[nodiscard]]). Discarding a free-function Subscription disconnects
///     the handler immediately (RAII).
///   - Subscription::reset() disconnects the connection for future delivery;
///     it does not depend on deleteLater() to unsubscribe and does not require
///     a running event loop. Per Qt queued-connection semantics, events already
///     posted to a still-alive receiver's queue may still be delivered after
///     reset(); new deliveries are prevented.
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
///   - aboutToQuit also sets BusRegistry::quitFired_. Publish/subscribe after
///     quit refuse to recreate dispatchers (qCCritical + no-op) rather than
///     constructing late unparented dispatchers against a dying application.
///   - Quit lifecycle re-arms per QCoreApplication instance: a process that
///     constructs a new QCoreApplication after a previous one quit gets a
///     fresh aboutToQuit hook and cleared quit flags. Hook state is
///     QPointer-tracked and reset on app destruction so the same instance is
///     never double-connected.
///
/// Threading
///   - Publish from the GUI thread only (template policy; no worker threads).
///   - The GUI-thread policy is enforced at runtime in all builds at publish
///     and both subscribe paths: wrong-thread calls log qCCritical(appEvent)
///     and are rejected (no-op / empty Subscription). Debug builds keep a
///     Q_ASSERT as a belt-and-suspenders double-check after the runtime
///     guard.
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
///
/// Qt moc constraint (do not "simplify" away QVariant transport)
///   - EventDispatcher<T> cannot use Q_OBJECT: Qt moc does not support
///     Q_OBJECT in class templates. The non-template EventDispatcherBase
///     therefore carries the Qt signal using QVariant transport. A typed
///     signal directly on EventDispatcher<T> is not viable under Qt 6 moc.
///   - The runtime meta-type check is intentional defense-in-depth at that
///     type-erasure boundary.
///
/// Exception policy
///   - Event handlers must not throw. Delivery uses Qt queued connections;
///     exceptions from handlers follow Qt slot semantics (undefined unless
///     handled in the handler). The EventSystem does not define a custom
///     exception framework.

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
    if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
      qCCritical(appEvent)
          << "EventSystem: publish requires the application thread";
      return;
    }
    Q_ASSERT(QThread::currentThread() ==
             QCoreApplication::instance()->thread());
    emit eventPublished(QVariant::fromValue(event));
  }
};

/// RAII subscription handle over QMetaObject::Connection.
///
/// Free-function subscriptions: destruction disconnects (default). The
/// returned Subscription must be retained; discarding it disconnects the
/// handler immediately.
///
/// QObject-receiver subscriptions: destruction does not disconnect
/// (receiver-lifetime mode via forReceiverLifetime). Connection lifetime
/// follows the receiver QObject; reset() still disconnects for early
/// unsubscribe.
class Subscription {
public:
  Subscription() noexcept = default;
  ~Subscription() {
    if (disconnectOnDestroy_) {
      reset();
    }
  }

  Subscription(const Subscription &) = delete;
  Subscription &operator=(const Subscription &) = delete;

  Subscription(Subscription &&other) noexcept
      : connection_(std::exchange(other.connection_, {})),
        disconnectOnDestroy_(other.disconnectOnDestroy_) {}

  Subscription &operator=(Subscription &&other) noexcept {
    if (this != &other) {
      reset();
      connection_ = std::exchange(other.connection_, {});
      disconnectOnDestroy_ = other.disconnectOnDestroy_;
    }
    return *this;
  }

  [[nodiscard]] bool isConnected() const {
    return static_cast<bool>(connection_);
  }

  /// Logical unsubscription: disconnect only. The connection context is
  /// application-owned (QCoreApplication) for free-function handles.
  /// reset() does not call deleteLater() and does not require a running
  /// event loop. Works for both free-function and receiver-lifetime handles.
  void reset() {
    if (static_cast<bool>(connection_)) {
      QObject::disconnect(connection_);
      connection_ = {};
    }
  }

  /// Free-function / general RAII handle: destruction disconnects.
  explicit Subscription(QMetaObject::Connection conn)
      : connection_(std::move(conn)), disconnectOnDestroy_(true) {}

  /// QObject-receiver lifetime handle: destruction does NOT disconnect.
  /// Qt tears the connection down when the receiver QObject is destroyed.
  /// Use when the caller intends the normal Qt receiver-lifetime pattern
  /// (handle may be discarded; early unsubscribe is still available via
  /// reset() while the handle is retained).
  static Subscription forReceiverLifetime(QMetaObject::Connection conn) {
    Subscription sub(std::move(conn));
    sub.disconnectOnDestroy_ = false;
    return sub;
  }

private:
  QMetaObject::Connection connection_;
  bool disconnectOnDestroy_ = true;
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
    if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
      qCCritical(appEvent)
          << "EventSystem: publish requires the application thread";
      return;
    }
    Q_ASSERT(QThread::currentThread() ==
             QCoreApplication::instance()->thread());
    {
      BusRegistry &reg = instance();
      std::unique_lock lock(reg.mutex_);
      if (reg.quitFired_) {
        qCCritical(appEvent)
            << "EventSystem: publish after application shutdown";
        return;
      }
    }
    dispatcher<T>().publish(event);
  }

  /// QObject-receiver subscribe. Returns a receiver-lifetime Subscription:
  /// connection lifetime follows the receiver; discarding the handle does
  /// not disconnect. reset() still disconnects for early unsubscribe.
  template <EventType T, typename Obj>
  static Subscription subscribe(Obj *receiver, void (Obj::*method)(const T &))
    requires(std::is_base_of_v<QObject, Obj>)
  {
    if (!QCoreApplication::instance()) {
      qCCritical(appEvent)
          << "EventSystem: QObject subscribe requires a running"
          << "QCoreApplication";
      return Subscription{};
    }
    if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
      qCCritical(appEvent)
          << "EventSystem: subscribe requires the application thread";
      return Subscription{};
    }
    Q_ASSERT(QThread::currentThread() ==
             QCoreApplication::instance()->thread());
    {
      BusRegistry &reg = instance();
      std::unique_lock lock(reg.mutex_);
      if (reg.quitFired_) {
        qCCritical(appEvent)
            << "EventSystem: subscribe after application shutdown";
        return Subscription{};
      }
    }
    auto conn = QObject::connect(
        &dispatcher<T>(), &EventDispatcherBase::eventPublished, receiver,
        [receiver, method](const QVariant &var) {
          T event{};
          if (!detail::checkedEventValue(var, event)) {
            return;
          }
          (receiver->*method)(event);
        },
        Qt::QueuedConnection);
    return Subscription::forReceiverLifetime(std::move(conn));
  }

  /// Free-function subscribe. Uses QCoreApplication::instance() as the
  /// connection context — no wrapper QObject is allocated. Free-function
  /// handlers execute on the application thread under the GUI-thread policy.
  /// Subscriptions belong to application lifetime; services::unregisterAll()
  /// is the explicit teardown mechanism for retained service subscriptions.
  /// Function-pointer-only: capturing lambdas/functors would reintroduce the
  /// lifetime hazards Subscription exists to avoid for stateless handlers;
  /// stateful handlers must use QObject receivers.
  template <EventType T>
  [[nodiscard]] static Subscription subscribe(void (*func)(const T &)) {
    QObject *context = QCoreApplication::instance();
    if (!context) {
      qCCritical(appEvent)
          << "EventSystem: free-function subscribe requires a running"
          << "QCoreApplication";
      return Subscription{};
    }
    if (QThread::currentThread() != context->thread()) {
      qCCritical(appEvent)
          << "EventSystem: subscribe requires the application thread";
      return Subscription{};
    }
    Q_ASSERT(QThread::currentThread() == context->thread());
    {
      BusRegistry &reg = instance();
      std::unique_lock lock(reg.mutex_);
      if (reg.quitFired_) {
        qCCritical(appEvent)
            << "EventSystem: subscribe after application shutdown";
        return Subscription{};
      }
    }
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

    // Structural lifetime guarantee:
    //   On first dispatcher creation for a given QCoreApplication instance,
    //   connect aboutToQuit to clear the non-owning map while QObject
    //   children (dispatchers) still exist. Qt then destroys the dispatcher
    //   objects with the application. The map never deletes or dereferences
    //   entries after that clear; adding a BusRegistry destructor that
    //   iterates+deletes would double-delete against Qt ownership.
    //
    // Quit-lifecycle re-arm (AUD-123 Option A):
    //   hookedApp_ is a QPointer — null after app destruction. When the
    //   current instance differs from the hooked instance (or no hook yet),
    //   clear quit flags and install aboutToQuit + destroyed handlers for
    //   the current app. Same-instance access never reinstalls (no
    //   double-connect). After aboutToQuit but before destruction,
    //   quitFired_ stays set for the dying instance — guards keep refusing.
    if (auto *app = QCoreApplication::instance()) {
      if (reg.hookedApp_.data() != app) {
        reg.quitFired_ = false;
        QObject::connect(app, &QCoreApplication::aboutToQuit, app, []() {
          BusRegistry &r = instance();
          std::unique_lock clearLock(r.mutex_);
          r.quitFired_ = true;
          r.dispatchers_.clear();
        });
        // Reset hook state when this app is destroyed so a subsequent
        // QCoreApplication reinstalls aboutToQuit (no double-connect).
        QObject::connect(app, &QObject::destroyed, app, []() {
          BusRegistry &r = instance();
          std::unique_lock clearLock(r.mutex_);
          r.quitHookInstalled_ = false;
          r.quitFired_ = false;
          r.hookedApp_.clear();
        });
        reg.quitHookInstalled_ = true;
        reg.hookedApp_ = app;
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
  // QPointer: null after QCoreApplication destruction. Compared against
  // QCoreApplication::instance() to detect app-instance change for re-arm.
  QPointer<QCoreApplication> hookedApp_;
  // Set under mutex_ when aboutToQuit clears dispatchers_. Publish/subscribe
  // check quitFired_ (under the same mutex) before calling dispatcher<T>(),
  // so a post-quit call cannot recreate dispatchers against a dying app.
  // Cleared when a new QCoreApplication instance appears (re-arm).
  bool quitFired_ = false;
};

/// QObject-receiver subscription. Returns a receiver-lifetime Subscription
/// (see Subscription::forReceiverLifetime). Discarding the handle is the
/// normal Qt pattern — connection lifetime follows the receiver; it does
/// not disconnect on handle destruction. reset() still disconnects for
/// early unsubscribe. Thin wrapper over BusRegistry::subscribe.
template <EventType T, typename Obj>
[[nodiscard]] Subscription subscribe(Obj *receiver,
                                     void (Obj::*method)(const T &))
  requires(std::is_base_of_v<QObject, Obj>)
{
  return BusRegistry::subscribe<T, Obj>(receiver, method);
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
/// Function-pointer-only by design: capturing lambdas/functors would
/// reintroduce the lifetime hazards Subscription exists to avoid for
/// stateless handlers. Stateful handlers must use QObject receivers (Qt
/// auto-disconnect on receiver destruction) or application-owned static
/// state that outlives the subscription.
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
