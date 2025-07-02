#pragma once

#include "../../events/system/EventSystem.hpp"
#include <QCoreApplication>
#include <QObject>
#include <optional>
#include <type_traits>

namespace services {

class ServiceRegistry {
public:
  static void registerAll();

  // Accepts only plain function pointers:
  template <typename InputEvent, typename OutputEvent>
  static void registerService(OutputEvent (*func)(InputEvent));

  template <typename InputEvent>
  static void registerOneWayService(void (*func)(const InputEvent&));

  template <typename InputEvent, typename OutputEvent>
  static void registerOptionalService(std::optional<OutputEvent> (*func)(const InputEvent&));
};

// === Implementations ===

template <typename InputEvent, typename OutputEvent>
void ServiceRegistry::registerService(OutputEvent (*func)(InputEvent)) {
  static QObject owner;
  events::subscribe<InputEvent>(&owner, [func](const InputEvent &inputEvent) {
    events::publish(func(inputEvent));
  });
}

template <typename InputEvent>
void ServiceRegistry::registerOneWayService(void (*func)(const InputEvent&)) {
  static QObject owner;
  events::subscribe<InputEvent>(&owner, [func](const InputEvent &inputEvent) {
    func(inputEvent);
  });
}

template <typename InputEvent, typename OutputEvent>
void ServiceRegistry::registerOptionalService(std::optional<OutputEvent> (*func)(const InputEvent&)) {
  static QObject owner;
  events::subscribe<InputEvent>(&owner, [func](const InputEvent &inputEvent) {
    std::optional<OutputEvent> out = func(inputEvent);
    if (out) {
      events::publish(*out);
    }
  });
}

} // namespace services