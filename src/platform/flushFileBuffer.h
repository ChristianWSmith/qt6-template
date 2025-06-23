#pragma once

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <QFile>

inline void flushToDisk(QFile &file) {
  file.flush();
  file.waitForBytesWritten(-1);
#ifdef _WIN32
  HANDLE hFile = (HANDLE)_get_osfhandle(file.handle());
  if (hFile != INVALID_HANDLE_VALUE) {
    FlushFileBuffers(hFile);
  }
#else
  ::fsync(file.handle());
#endif
}
