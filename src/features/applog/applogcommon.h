#pragma once

#include <QString>

class AppLogTest;

struct LogDelta {
  QString message;
  bool trimmed;

  bool operator==(const LogDelta &other) const {
    return message == other.message && trimmed == other.trimmed;
  }
};
