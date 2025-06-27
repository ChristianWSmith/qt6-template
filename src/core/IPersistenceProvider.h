//---core/IPersistenceProvider.h---
#pragma once
#include <QJsonObject>
#include <QObject>
#include <QtPlugin>

class IPersistenceProvider {
public:
  IPersistenceProvider() = default;
  virtual ~IPersistenceProvider() = default;

  IPersistenceProvider(const IPersistenceProvider &) = delete;
  IPersistenceProvider &operator=(const IPersistenceProvider &) = delete;
  IPersistenceProvider(IPersistenceProvider &&) = delete;
  IPersistenceProvider &operator=(IPersistenceProvider &&) = delete;

  virtual QJsonObject loadState(const QString &key) = 0;
  virtual void saveState(const QString &key, const QJsonObject &state) = 0;
};

#define IPersistenceProvider_iid APP_ID ".IPersistenceProvider"
Q_DECLARE_INTERFACE(IPersistenceProvider, IPersistenceProvider_iid)
