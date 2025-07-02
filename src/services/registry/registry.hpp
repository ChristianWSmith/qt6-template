#pragma once

#include "../../events/system/EventSystem.hpp"
#include "../ConsoleLogService.hpp"
#include <QCoreApplication>
#include <QObject>

namespace services {

void registerAll() { events::subscribe<LogEvent>(ConsoleLogService::handle); }

} // namespace services