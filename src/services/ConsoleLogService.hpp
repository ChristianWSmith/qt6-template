#pragma once

#include "../events/LogEvent.h"
#include "../logging/logging.h"
#include <QDebug>

namespace ConsoleLogService {

inline void handle(const LogEvent &event) {
  qCInfo(appService) << "ConsoleLogService:" << QString::fromStdString(event.message);
}

} // namespace ConsoleLogService
