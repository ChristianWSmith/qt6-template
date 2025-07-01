#pragma once

#include "../../events/system/EventSystem.hpp"
#include <QObject>
#include <optional>

namespace services {

class ServiceRegistry {
public:
  static void registerAll();

  template <typename InputEvent, typename OutputEvent, typename Callable>
  static void registerService(Callable &&func);

  template <typename InputEvent, typename Callable>
  static void registerOneWayService(Callable &&func);

  template <typename InputEvent, typename OutputEvent, typename Callable>
  static void registerOptionalService(Callable &&func);

private:
  static QObject *serviceOwner();
};

// Singleton QObject for lifetime management
inline QObject *ServiceRegistry::serviceOwner() {
  static QObject owner; // Lives until program shutdown
  return &owner;
}

template <typename InputEvent, typename OutputEvent, typename Callable>
void ServiceRegistry::registerService(Callable &&func) {
  events::subscribe<InputEvent>(
      serviceOwner(),
      [func = std::forward<Callable>(func)](const InputEvent &inputEvent) {
        events::publish<OutputEvent>(func(inputEvent));
      });
}

template <typename InputEvent, typename Callable>
void ServiceRegistry::registerOneWayService(Callable &&func) {
  events::subscribe<InputEvent>(
      serviceOwner(), [func = std::forward<Callable>(func)](
                          const InputEvent &inputEvent) { func(inputEvent); });
}

template <typename InputEvent, typename OutputEvent, typename Callable>
void ServiceRegistry::registerOptionalService(Callable &&func) {
  events::subscribe<InputEvent>(
      serviceOwner(),
      [func = std::forward<Callable>(func)](const InputEvent &inputEvent) {
        std::optional<OutputEvent> out = func(inputEvent);
        if (out) {
          events::publish<OutputEvent>(*out);
        }
      });
}

} // namespace services
