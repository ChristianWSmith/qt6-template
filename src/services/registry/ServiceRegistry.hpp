#pragma once

#include "../../events/bus/EventBus.hpp"
#include <memory>
#include <optional>
#include <vector>

namespace services {

class ServiceRegistry {
public:
  static void registerAll();

private:
  static std::vector<std::shared_ptr<void>> &subscriptions_();
  template <typename InputEvent, typename OutputEvent, typename Callable>
  static void registerService(Callable &&func);

  template <typename InputEvent, typename Callable>
  static void registerOneWayService(Callable &&func);

  template <typename InputEvent, typename OutputEvent, typename Callable>
  static void registerOptionalService(Callable &&func);
};

template <typename InputEvent, typename OutputEvent, typename Callable>
events::Subscription<InputEvent> connectService(Callable &&func) {
  return events::subscribe<InputEvent>(
      [func = std::forward<Callable>(func)](const InputEvent &inputEvent) {
        OutputEvent out = func(inputEvent);
        events::publish<OutputEvent>(out);
      });
}

template <typename InputEvent, typename Callable>
events::Subscription<InputEvent> connectOneWayService(Callable &&func) {
  return events::subscribe<InputEvent>(
      [func = std::forward<Callable>(func)](const InputEvent &inputEvent) {
        func(inputEvent);
      });
}

template <typename InputEvent, typename OutputEvent, typename Callable>
events::Subscription<InputEvent> connectOptionalService(Callable &&func) {
  return events::subscribe<InputEvent>(
      [func = std::forward<Callable>(func)](const InputEvent &inputEvent) {
        std::optional<OutputEvent> out = func(inputEvent);
        if (out) {
          events::publish<OutputEvent>(*out);
        }
      });
}

template <typename InputEvent, typename OutputEvent, typename Callable>
void ServiceRegistry::registerService(Callable &&func) {
  subscriptions_().push_back(std::make_shared<events::Subscription<InputEvent>>(
      connectService<InputEvent, OutputEvent>(std::forward<Callable>(func))));
}

template <typename InputEvent, typename Callable>
void ServiceRegistry::registerOneWayService(Callable &&func) {
  subscriptions_().push_back(std::make_shared<events::Subscription<InputEvent>>(
      connectOneWayService<InputEvent>(std::forward<Callable>(func))));
}

template <typename InputEvent, typename OutputEvent, typename Callable>
void ServiceRegistry::registerOptionalService(Callable &&func) {
  subscriptions_().push_back(std::make_shared<events::Subscription<InputEvent>>(
      connectOptionalService<InputEvent, OutputEvent>(
          std::forward<Callable>(func))));
}

} // namespace services
