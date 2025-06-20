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
  static void registerService(Callable &&fn);

  template <typename InputEvent, typename Callable>
  static void registerOneWayService(Callable &&fn);

  template <typename InputEvent, typename OutputEvent, typename Callable>
  static void registerOptionalService(Callable &&fn);
};

template <typename InputEvent, typename OutputEvent, typename Callable>
events::Subscription<InputEvent> connectService(Callable &&fn) {
  return events::subscribe<InputEvent>(
      [fn = std::forward<Callable>(fn)](const InputEvent &in) {
        OutputEvent out = fn(in);
        events::publish<OutputEvent>(out);
      });
}

template <typename InputEvent, typename Callable>
events::Subscription<InputEvent> connectOneWayService(Callable &&fn) {
  return events::subscribe<InputEvent>(
      [fn = std::forward<Callable>(fn)](const InputEvent &in) { fn(in); });
}

template <typename InputEvent, typename OutputEvent, typename Callable>
events::Subscription<InputEvent> connectOptionalService(Callable &&fn) {
  return events::subscribe<InputEvent>(
      [fn = std::forward<Callable>(fn)](const InputEvent &in) {
        std::optional<OutputEvent> out = fn(in);
        if (out) {
          events::publish<OutputEvent>(*out);
        }
      });
}

template <typename InputEvent, typename OutputEvent, typename Callable>
void ServiceRegistry::registerService(Callable &&fn) {
  subscriptions_().push_back(std::make_shared<events::Subscription<InputEvent>>(
      connectService<InputEvent, OutputEvent>(std::forward<Callable>(fn))));
}

template <typename InputEvent, typename Callable>
void ServiceRegistry::registerOneWayService(Callable &&fn) {
  subscriptions_().push_back(std::make_shared<events::Subscription<InputEvent>>(
      connectOneWayService<InputEvent>(std::forward<Callable>(fn))));
}

template <typename InputEvent, typename OutputEvent, typename Callable>
void ServiceRegistry::registerOptionalService(Callable &&fn) {
  subscriptions_().push_back(std::make_shared<events::Subscription<InputEvent>>(
      connectOptionalService<InputEvent, OutputEvent>(
          std::forward<Callable>(fn))));
}

} // namespace services
