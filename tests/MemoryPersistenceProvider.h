#pragma once

#include "core/IPersistenceProvider.h"
#include <QJsonDocument>
#include <QMap>

class MemoryPersistenceProvider : public IPersistenceProvider {
public:
  PersistenceResult<QJsonObject> loadState(const QString &key) override {
    auto it = storage_.find(key);
    if (it == storage_.end()) {
      return PersistenceResult<QJsonObject>::failure(PersistenceError::NotFound);
    }
    QJsonDocument doc = QJsonDocument::fromJson(it.value());
    if (doc.isNull()) {
      return PersistenceResult<QJsonObject>::failure(PersistenceError::InvalidData);
    }
    return PersistenceResult<QJsonObject>::success(doc.object());
  }

  PersistenceResult<void> saveState(const QString &key, const QJsonObject &state) override {
    storage_[key] = QJsonDocument(state).toJson(QJsonDocument::Compact);
    return PersistenceResult<void>::success();
  }

private:
  QMap<QString, QByteArray> storage_;
};
