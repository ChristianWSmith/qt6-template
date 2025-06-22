#pragma once

#include <QString>
#define APPLOG_FEATURE_ID APP_ID ".AppLog"

struct LogDelta {
  QString message;
  bool trimmed;
};
