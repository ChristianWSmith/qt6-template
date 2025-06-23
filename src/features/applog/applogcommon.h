#pragma once

#include <QString>
#define APPLOG_FEATURE_ID APP_ID ".AppLog"

struct LogDelta {
  QString message;
  bool trimmed;

  bool operator==(const LogDelta &other) const {
    return message == other.message && trimmed == other.trimmed;
  }
};
