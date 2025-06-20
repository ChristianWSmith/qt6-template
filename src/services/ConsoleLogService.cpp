#include "ConsoleLogService.hpp"
#include <QDebug>

namespace ConsoleLogService {

void handle(const LogEvent &event) {
  qInfo() << "ConsoleLogService:" << QString::fromStdString(event.message);
}

} // namespace ConsoleLogService
