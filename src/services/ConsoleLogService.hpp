#pragma once

#include "../events/LogEvent.h"
#include "../logging/logging.h"
#include <QDebug>

// Demonstrates free-function EventSystem subscription via services::registerAll().
// Production apps do not publish LogEvent; this service is template scaffolding
// for the registration pattern (see AGENTS.md EventSystem / Service Registration).

namespace ConsoleLogService {

inline void handle(const LogEvent &event) {
  qCInfo(appService) << "ConsoleLogService:" << QString::fromStdString(event.message);
}

} // namespace ConsoleLogService
