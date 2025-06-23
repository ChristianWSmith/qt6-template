#pragma once

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <QFile>

inline void flushToDisk(QFile &file) {
  file.flush();
  file.waitForBytesWritten(-1);
#ifdef _WIN32
  HANDLE h = reinterpret_cast<HANDLE>(_get_osfhandle(file.handle()));
  if (h != INVALID_HANDLE_VALUE) {
    FlushFileBuffers(h);
  }
#else
  ::fsync(file.handle());
#endif
}
