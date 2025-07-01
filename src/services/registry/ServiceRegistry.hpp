#pragma once

#include "../../events/system/EventSystem.hpp"
#include <memory>
#include <optional>
#include <vector>

namespace services {

class ServiceRegistry {
public:
  static void registerAll();

  static std::vector<void> &subscriptions_();

  template <typename InputEvent, typename OutputEvent, typename Callable>
  static void registerService(Callable &&func);

  template <typename InputEvent, typename Callable>
  static void registerOneWayService(Callable &&func);

  template <typename InputEvent, typename OutputEvent, typename Callable>
  static void registerOptionalService(Callable &&func);
};

template <typename InputEvent, typename OutputEvent, typename Callable>
void connectService(Callable &&func) {
  events::subscribe<InputEvent>(
      [func = std::forward<Callable>(func)](const InputEvent &inputEvent) {
        OutputEvent out = func(inputEvent);
        events::BusRegistry::publish<OutputEvent>(out);
      });
}

template <typename InputEvent, typename Callable>
void connectOneWayService(Callable &&func) {
  events::subscribe<InputEvent>(
      [func = std::forward<Callable>(func)](const InputEvent &inputEvent) {
        func(inputEvent);
      });
}

template <typename InputEvent, typename OutputEvent, typename Callable>
void connectOptionalService(Callable &&func) {
  events::subscribe<InputEvent>(
      [func = std::forward<Callable>(func)](const InputEvent &inputEvent) {
        std::optional<OutputEvent> out = func(inputEvent);
        if (out) {
          events::BusRegistry::publish<OutputEvent>(*out);
        }
      });
}

template <typename InputEvent, typename OutputEvent, typename Callable>
void ServiceRegistry::registerService(Callable &&func) {
  connectService<InputEvent, OutputEvent>(std::forward<Callable>(func));
}

template <typename InputEvent, typename Callable>
void ServiceRegistry::registerOneWayService(Callable &&func) {
  connectOneWayService<InputEvent>(std::forward<Callable>(func));
}

template <typename InputEvent, typename OutputEvent, typename Callable>
void ServiceRegistry::registerOptionalService(Callable &&func) {
  connectOptionalService<InputEvent, OutputEvent>(std::forward<Callable>(func));
}

} // namespace services
