#pragma once

#include "../events/LogEvent.h"
#include "../logging/logging.h"
#include <QDebug>
#include <atomic>

namespace ConsoleLogService {

// Test observability: incremented on every delivered LogEvent.
inline std::atomic<int> receivedCount{0};

inline void handle(const LogEvent &event) {
  receivedCount.fetch_add(1, std::memory_order_relaxed);
  qCInfo(appService) << "ConsoleLogService:" << QString::fromStdString(event.message);
}

} // namespace ConsoleLogService
