#include "ServiceRegistry.hpp"
#include "../../events/LogEvent.h"
#include "../ConsoleLogService.hpp"

namespace services {

void ServiceRegistry::registerAll() {
  registerOneWayService<LogEvent>(ConsoleLogService::handle);
}

} // namespace services
