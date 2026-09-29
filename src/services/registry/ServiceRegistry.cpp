#include "ServiceRegistry.hpp"
#include "../ConsoleLogService.hpp"
#include "../../events/LogEvent.h"

namespace services {

void registerAll() {
    events::subscribe<LogEvent>(ConsoleLogService::handle);
}

} // namespace services
