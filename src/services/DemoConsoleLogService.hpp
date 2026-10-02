#pragma once

#include "../events/DemoLogEvent.h"
#include "../logging/logging.h"
#include <QDebug>

// Demonstration only — no production publisher of DemoLogEvent. Features use
// Qt signals for real data. This service demonstrates free-function
// EventSystem subscription via services::registerAll() (template scaffolding;
// see AGENTS.md EventSystem / Service Registration / Production status).

namespace DemoConsoleLogService {

inline void handle(const DemoLogEvent &event) {
  qCInfo(appService) << "DemoConsoleLogService:"
                     << QString::fromStdString(event.message);
}

} // namespace DemoConsoleLogService
