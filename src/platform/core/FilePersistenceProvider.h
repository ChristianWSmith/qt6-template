#pragma once
#include "../../core/IPersistenceProvider.h"

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <QFile>

inline bool flushToDisk(QFile &file) {
  if (!file.flush()) {
    return false;
  }
  if (!file.waitForBytesWritten(-1)) {
    return false;
  }
#ifdef Q_OS_WIN
  HANDLE h = reinterpret_cast<HANDLE>(_get_osfhandle(file.handle()));
  if (h != INVALID_HANDLE_VALUE) {
    return FlushFileBuffers(h) != 0;
  }
  return false;
#else
  return ::fsync(file.handle()) == 0;
#endif
}

class FilePersistenceProvider : public QObject, public IPersistenceProvider {
  Q_OBJECT

public:
  explicit FilePersistenceProvider(QObject *parent = nullptr);
  PersistenceResult<QJsonObject> loadState(const QString &key) override;
  PersistenceResult<void> saveState(const QString &key,
                                    const QJsonObject &state) override;
};
