#pragma once

#include "../../events/system/EventSystem.hpp"
#include <QCoreApplication>
#include <QObject>
#include <optional>

namespace services {

class ServiceRegistry {
public:
  static void registerAll();

  // Enforce function pointer signatures with const-ref input
  template <typename InputEvent, typename OutputEvent>
  static void registerService(OutputEvent (*func)(const InputEvent&));

  template <typename InputEvent>
  static void registerOneWayService(void (*func)(const InputEvent&));

  template <typename InputEvent, typename OutputEvent>
  static void registerOptionalService(std::optional<OutputEvent> (*func)(const InputEvent&));
};

// === Implementations ===

template <typename InputEvent, typename OutputEvent>
void ServiceRegistry::registerService(OutputEvent (*func)(const InputEvent&)) {
  static QObject owner;
  events::subscribe<InputEvent>(&owner, [func](const InputEvent& inputEvent) {
    events::publish(func(inputEvent));
  });
}

template <typename InputEvent>
void ServiceRegistry::registerOneWayService(void (*func)(const InputEvent&)) {
  static QObject owner;
  events::subscribe<InputEvent>(&owner, [func](const InputEvent& inputEvent) {
    func(inputEvent);
  });
}

template <typename InputEvent, typename OutputEvent>
void ServiceRegistry::registerOptionalService(std::optional<OutputEvent> (*func)(const InputEvent&)) {
  static QObject owner;
  events::subscribe<InputEvent>(&owner, [func](const InputEvent& inputEvent) {
    if (auto out = func(inputEvent)) {
      events::publish(*out);
    }
  });
}

} // namespace services