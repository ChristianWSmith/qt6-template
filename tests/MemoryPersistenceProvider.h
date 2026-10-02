#pragma once

#include "core/IPersistenceProvider.h"
#include <QJsonDocument>
#include <QMap>
#include <optional>

/// In-memory IPersistenceProvider test double.
/// Supports optional one-shot failure injection so model/provider error
/// paths can be exercised without touching the filesystem.
///
/// Load taxonomy parity with FilePersistenceProvider:
///   - missing key → NotFound (first-run)
///   - corrupt JSON (parse error) → InvalidData
///   - valid JSON that is not an object → InvalidData
/// This is an API-boundary double, not a storage-behavior clone
/// (no filesystem, no QSaveFile commit semantics).
class MemoryPersistenceProvider : public IPersistenceProvider {
public:
  void failNextLoad(PersistenceError error) { nextLoadError_ = error; }
  void failNextSave(PersistenceError error) { nextSaveError_ = error; }

  /// Test-only: inject raw bytes to simulate damaged or non-object storage
  /// that cannot be produced through the public QJsonObject saveState API.
  void seedRaw(const QString &key, const QByteArray &bytes) {
    storage_[key] = bytes;
  }

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
    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(it.value(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
      return PersistenceResult<QJsonObject>::failure(
          PersistenceError::InvalidData);
    }
    if (!doc.isObject()) {
      return PersistenceResult<QJsonObject>::failure(
          PersistenceError::InvalidData);
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
