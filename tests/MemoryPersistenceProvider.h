#pragma once

#include "core/IPersistenceProvider.h"
#include <QJsonDocument>
#include <QMap>
#include <optional>

/// In-memory IPersistenceProvider test double.
/// Supports optional one-shot failure injection so model/provider error
/// paths can be exercised without touching the filesystem.
///
/// Fidelity notes (G-007):
///   - Stores compact JSON objects only; cannot naturally produce
///     FilePersistenceProvider's on-disk InvalidData for arrays written by
///     external writers (saveState always writes a valid JSON object).
///   - Error kinds other than NotFound are available via failNextLoad /
///     failNextSave injection (CommitError, IoError, InvalidData).
///   - Not thread-safe — matches the GUI-thread persistence contract.
class MemoryPersistenceProvider : public IPersistenceProvider {
public:
  void failNextLoad(PersistenceError error) { nextLoadError_ = error; }
  void failNextSave(PersistenceError error) { nextSaveError_ = error; }

  PersistenceResult<QJsonObject> loadState(const QString &key) override {
    if (nextLoadError_.has_value()) {
      const PersistenceError error = *nextLoadError_;
      nextLoadError_.reset();
      return PersistenceResult<QJsonObject>::failure(error);
    }
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
    if (nextSaveError_.has_value()) {
      const PersistenceError error = *nextSaveError_;
      nextSaveError_.reset();
      return PersistenceResult<void>::failure(error);
    }
    storage_[key] = QJsonDocument(state).toJson(QJsonDocument::Compact);
    return PersistenceResult<void>::success();
  }

private:
  QMap<QString, QByteArray> storage_;
  std::optional<PersistenceError> nextLoadError_;
  std::optional<PersistenceError> nextSaveError_;
};
