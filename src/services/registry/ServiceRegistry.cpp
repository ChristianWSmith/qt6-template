#include "ServiceRegistry.hpp"
#include "../ConsoleLogService.hpp"
#include "../../events/LogEvent.h"

namespace services {

namespace {
// Process-lifetime storage: free-function subscribe returns an RAII
// Subscription that must be held for the subscription to remain active.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
events::Subscription g_consoleLogSubscription;
} // namespace

void registerAll() {
    g_consoleLogSubscription =
        events::subscribe<LogEvent>(ConsoleLogService::handle);
}

} // namespace services
