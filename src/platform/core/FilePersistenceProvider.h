#pragma once
#include "../../core/IPersistenceProvider.h"

#include <QFile>

class FilePersistenceProvider : public QObject, public IPersistenceProvider {
  Q_OBJECT

public:
  explicit FilePersistenceProvider(QObject *parent = nullptr);
  PersistenceResult<QJsonObject> loadState(const QString &key) override;
  PersistenceResult<void> saveState(const QString &key,
                                    const QJsonObject &state) override;
};
