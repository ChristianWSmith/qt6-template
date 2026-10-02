#pragma once

#include <QCoreApplication>
#include <QObject>
#include <QThread>
#include <QVariant>
#include <concepts>
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
///   - The smoke-test path in main() may never call QApplication::exec(), so
///     aboutToQuit may never fire. That path must call services::unregisterAll()
///     and must not publish after teardown.
///
/// Single-application constraint
///   - BusRegistry is process-global and keyed to a single QCoreApplication
///     lifetime. A second QApplication in the same process is not supported:
///     quitFired_ / quitHookInstalled_ are sticky and the dispatcher map is
///     not reset for a new application. One QApplication per process is a hard
///     requirement for this template.
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
///   - There is no BusRegistry mutex. Under the GUI-thread-only policy all
///     registry state is already serialized by the event loop; a mutex would
///     imply a cross-thread safety contract the bus does not provide.
///   - The bus is not a general cross-thread synchronization mechanism.
///
/// Event payload copy cost
///   - QVariant transport copies each event once into the QVariant and once
///     per subscriber (var.value<T>()). Choose event payload sizes
///     accordingly; large payloads on a hot path are a design smell.
///
/// Service registration lifecycle
///   - QApplication exists → services::registerAll() → application runs →
///     services::unregisterAll() → QApplication destruction.
///   - unregisterAll() resets retained subscriptions explicitly; it is
///     idempotent and independent of static destructor timing.
///   - On C++ exception paths, qScopeGuard runs during stack unwinding before
///     catch handlers; catch-block unregisterAll() calls are idempotent no-ops
///     after ~QApplication. The guard is the exception-path mechanism.
///
/// API boundary
///   - Application code should use events::publish and events::subscribe
///     (free-function and QObject overloads). These are the supported
///     public-facing entry points.
///   - BusRegistry also exposes equivalent static publish/subscribe
///     operations; consumers should use the free-function facade.
///   - BusRegistry::dispatcher<T>() is an internal implementation detail
///     (private). Do not call it or treat it as public API.
///   - Runtime QVariant type check remains as defense-in-depth against
///     QVariant corruption; the primary protection is the typed public API
///     plus the private dispatcher boundary.
///
/// Type safety
///   - Event types must be default-constructible, copy-constructible, and
///     copy-assignable (compile-time; EventType concept).
///   - Delivery checks meta-type in all builds; debug builds also assert.
///     Mismatch logs qCritical(appEvent) and does not deliver.
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

  // Inner instance/thread guards are belt-and-suspenders for white-box
  // findChildren escape hatches; the public policy checks live in
  // BusRegistry::{publish,subscribe}.
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
/// (thin wrappers over the static methods below). dispatcher<T>() returns a
/// pointer and is an implementation detail — not a public mutable-ref escape
/// hatch. Returns nullptr when quitFired_ is set or no QCoreApplication
/// exists (post-teardown / smoke path without exec()).
class BusRegistry {
public:
  template <EventType T> static void publish(const T &event) {
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
    if (instance().quitFired_) {
      qCCritical(appEvent) << "EventSystem: publish after application shutdown";
      return;
    }
    EventDispatcher<T> *disp = dispatcher<T>();
    if (!disp) {
      qCCritical(appEvent)
          << "EventSystem: publish after application shutdown";
      return;
    }
    disp->publish(event);
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
    if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
      qCCritical(appEvent)
          << "EventSystem: subscribe requires the application thread";
      return;
    }
    Q_ASSERT(QThread::currentThread() ==
             QCoreApplication::instance()->thread());
    if (instance().quitFired_) {
      qCCritical(appEvent)
          << "EventSystem: subscribe after application shutdown";
      return;
    }
    EventDispatcher<T> *disp = dispatcher<T>();
    if (!disp) {
      qCCritical(appEvent)
          << "EventSystem: subscribe after application shutdown";
      return;
    }
    QObject::connect(
        disp, &EventDispatcherBase::eventPublished, receiver,
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
    if (QThread::currentThread() != context->thread()) {
      qCCritical(appEvent)
          << "EventSystem: subscribe requires the application thread";
      return Subscription{};
    }
    Q_ASSERT(QThread::currentThread() == context->thread());
    if (instance().quitFired_) {
      qCCritical(appEvent)
          << "EventSystem: subscribe after application shutdown";
      return Subscription{};
    }
    EventDispatcher<T> *disp = dispatcher<T>();
    if (!disp) {
      qCCritical(appEvent)
          << "EventSystem: subscribe after application shutdown";
      return Subscription{};
    }
    auto conn = QObject::connect(
        disp, &EventDispatcherBase::eventPublished, context,
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
  /// Returns nullptr when quitFired_ is set or no QCoreApplication exists —
  /// callers must treat that as shutdown refusal (no dispatcher recreation).
  template <EventType T> static EventDispatcher<T> *dispatcher() {
    BusRegistry &reg = instance();
    if (reg.quitFired_) {
      return nullptr;
    }

    // Structural lifetime guarantee:
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
              r.quitFired_ = true;
              r.dispatchers_.clear();
            });
        reg.quitHookInstalled_ = true;
      } else {
        return nullptr;
      }
    }

    // Non-owning: QObject parent (QCoreApplication) owns the dispatcher when
    // one exists. Raw pointer avoids double-delete against Qt parent/child.
    EventDispatcherBase *&basePtr = reg.dispatchers_[typeid(T)];
    if (!basePtr) {
      auto *app = QCoreApplication::instance();
      if (!app || reg.quitFired_) {
        return nullptr;
      }
      // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
      auto *typed = new EventDispatcher<T>(app);
      basePtr = typed;
      qRegisterMetaType<T>();
    }

    return static_cast<EventDispatcher<T> *>(basePtr);
  }

  static BusRegistry &instance() {
    static BusRegistry busRegistry;
    return busRegistry;
  }

  std::unordered_map<std::type_index, EventDispatcherBase *> dispatchers_;
  bool quitHookInstalled_ = false;
  // Set on aboutToQuit when the map is cleared. Plain bool: GUI-thread-only
  // policy means no concurrent writer. Publish/subscribe check this before
  // dispatcher<T>(), which also refuses to create when set.
  bool quitFired_ = false;
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
template <EventType T> void publish(const T &event) {
  BusRegistry::publish<T>(event);
}

} // namespace events
