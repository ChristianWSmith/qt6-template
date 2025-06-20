#include "ConsoleLogService.hpp"
#include <QDebug>

namespace ConsoleLogService {

void handle(const LogEvent &event) {
  qInfo() << "ConsoleLogService:" << event.message;
}

} // namespace ConsoleLogService
