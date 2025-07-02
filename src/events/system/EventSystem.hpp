#pragma once

#include <QObject>
#include <QPointer>
#include <QVariant>
#include <functional>
#include <memory>
#include <mutex>
#include <typeindex>
#include <unordered_map>

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

template <typename T> class LambdaWrapper : public QObject {
public:
  explicit LambdaWrapper(std::function<void(const T &)> func,
                         QObject *parent = nullptr)
      : QObject(parent), func_(std::move(func)) {}

  void handle(const QVariant &var) { func_(var.value<T>()); }

private:
  std::function<void(const T &)> func_;
};

template <typename T, typename Obj>
void subscribe(Obj *receiver, void (Obj::*method)(const T &))
  requires(std::is_base_of_v<QObject, Obj>)
{
  QObject::connect(
      &BusRegistry::dispatcher<T>(), &EventDispatcherBase::eventPublished,
      receiver,
      [receiver, method](const QVariant &var) {
        (receiver->*method)(var.value<T>());
      },
      Qt::QueuedConnection);
}

template <typename T>
void subscribe(QObject *owner, std::function<void(const T &)> func) {
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
  auto *wrapper = new LambdaWrapper<T>(std::move(func), owner);
  QObject::connect(&BusRegistry::dispatcher<T>(),
                   &EventDispatcherBase::eventPublished, wrapper,
                   &LambdaWrapper<T>::handle, Qt::QueuedConnection);
}

template <typename T> void publish(T event) {
  BusRegistry::publish<T>(std::move(event));
}

} // namespace events
