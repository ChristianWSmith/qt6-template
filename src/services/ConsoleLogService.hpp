#pragma once

#include "../events/LogEvent.h"
#include "../logging/logging.h"
#include <QDebug>
#include <atomic>

// Demonstrates free-function EventSystem subscription via services::registerAll().
// Production apps do not publish LogEvent; this service is template scaffolding
// for the registration pattern (see AGENTS.md EventSystem / Service Registration).

namespace ConsoleLogService {

// Test observability: incremented on every delivered LogEvent.
inline std::atomic<int> receivedCount{0};

inline void handle(const LogEvent &event) {
  receivedCount.fetch_add(1, std::memory_order_relaxed);
  qCInfo(appService) << "ConsoleLogService:" << QString::fromStdString(event.message);
}

} // namespace ConsoleLogService
