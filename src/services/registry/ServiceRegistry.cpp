#include "ServiceRegistry.hpp"

namespace services {

void registerAll() {
  for (auto func : detail::registry()) {
    func();
  }
}

} // namespace services
