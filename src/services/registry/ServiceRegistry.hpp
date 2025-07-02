#pragma once

#include "../../events/system/EventSystem.hpp"
#include <vector>

struct Dummy {};
static_assert(std::is_same_v<decltype(events::publish<Dummy>), void(Dummy)>);

namespace services {
void registerAll(); // Forward declare so we can call it later
}

namespace services::detail {

using RegisterFunction = void (*)();

inline std::vector<RegisterFunction> &registry() {
  static std::vector<RegisterFunction> fns;
  return fns;
}

struct Registrar {
  Registrar(RegisterFunction func) { registry().push_back(func); }
};

} // namespace services::detail

// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define REGISTER_SERVICE(FUNC)                                                 \
  namespace {                                                                  \
  static const services::detail::Registrar _registrar_##__COUNTER__([]() {     \
    events::subscribe(std::move(FUNC));                                        \
  });                                                                          \
  }
// NOLINTEND(cppcoreguidelines-macro-usage)
