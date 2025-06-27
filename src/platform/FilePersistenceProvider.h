//---platform/FilePersistenceProvider.h---
#pragma once
#include "../core/IPersistenceProvider.h"

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

class FilePersistenceProvider : public QObject, public IPersistenceProvider {
  Q_OBJECT
  Q_PLUGIN_METADATA(IID IPersistenceProvider_iid)
  Q_INTERFACES(IPersistenceProvider)

public:
  QJsonObject loadState(const QString &key) override;
  void saveState(const QString &key, const QJsonObject &state) override;
};
