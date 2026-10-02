#pragma once

#include <QString>

inline constexpr int kMaxLogSize = 100;

struct LogDelta {
  QString message;
  bool trimmed;

  bool operator==(const LogDelta &other) const {
    return message == other.message && trimmed == other.trimmed;
  }
};
