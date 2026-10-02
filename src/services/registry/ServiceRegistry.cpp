#include "ServiceRegistry.hpp"
#include "../DemoConsoleLogService.hpp"
#include "../../events/DemoLogEvent.h"
#include "../../events/system/EventSystem.hpp"

namespace services {

namespace {
// Storage only — not the lifecycle mechanism. registerAll() assigns a
// free-function Subscription; unregisterAll() explicitly resets it before
// QApplication destruction, independent of static destructor timing.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
events::Subscription g_demoConsoleLogSubscription;
} // namespace

void registerAll() {
    g_demoConsoleLogSubscription =
        events::subscribe<DemoLogEvent>(DemoConsoleLogService::handle);
}

void unregisterAll() {
    g_demoConsoleLogSubscription.reset();
}

} // namespace services
