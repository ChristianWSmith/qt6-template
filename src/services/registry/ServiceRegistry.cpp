#include "ServiceRegistry.hpp"
#include "../../events/LogEvent.h"
#include "../ConsoleLogService.hpp"

namespace services {

void ServiceRegistry::registerAll() {
  registerOneWayService<LogEvent>(ConsoleLogService::handle);
}

std::vector<std::shared_ptr<void>> &ServiceRegistry::subscriptions_() {
  static auto *vec = new std::vector<std::shared_ptr<void>>();
  return *vec;
}

} // namespace services
