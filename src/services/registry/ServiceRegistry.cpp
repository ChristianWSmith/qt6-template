#include "ServiceRegistry.hpp"
#include "../ConsoleLogService.hpp"
#include "../../events/LogEvent.h"

namespace services {

namespace {
// Storage only — not the lifecycle mechanism. registerAll() assigns a
// free-function Subscription; unregisterAll() explicitly resets it before
// QApplication destruction, independent of static destructor timing.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
events::Subscription g_consoleLogSubscription;
} // namespace

void registerAll() {
    g_consoleLogSubscription =
        events::subscribe<LogEvent>(ConsoleLogService::handle);
}

void unregisterAll() {
    g_consoleLogSubscription.reset();
}

} // namespace services
