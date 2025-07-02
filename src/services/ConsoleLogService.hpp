#pragma once
#include "../events/LogEvent.h"
#include "registry/ServiceRegistry.hpp"
#include <QDebug>

namespace ConsoleLogService {

void handle(const LogEvent &event) {
  qInfo() << "ConsoleLogService:" << QString::fromStdString(event.message);
}

} // namespace ConsoleLogService

REGISTER_SERVICE(ConsoleLogService::handle);
