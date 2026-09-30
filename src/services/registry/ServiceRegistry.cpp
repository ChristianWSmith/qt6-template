#include "ServiceRegistry.hpp"
#include "../ConsoleLogService.hpp"
#include "../../events/LogEvent.h"

namespace services {

namespace {
// Process-lifetime storage: free-function subscribe returns an RAII
// Subscription that must be held for the subscription to remain active.
events::Subscription g_consoleLogSubscription;
} // namespace

void registerAll() {
    g_consoleLogSubscription =
        events::subscribe<LogEvent>(ConsoleLogService::handle);
}

} // namespace services
