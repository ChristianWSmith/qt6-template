#pragma once

#include <QObject>
#include <string>

struct LogEvent {
  std::string message;
};

Q_DECLARE_METATYPE(LogEvent)
