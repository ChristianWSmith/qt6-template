#include "FilePersistenceProvider.h"
#include "../../logging/logging.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>
#include <QThread>

FilePersistenceProvider::FilePersistenceProvider(QObject *parent)
    : QObject(parent) {}

static QString getFilePath(const QString &key) {
  return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
         "/" + key + ".json";
}

PersistenceResult<QJsonObject>
FilePersistenceProvider::loadState(const QString &key) {
  // Debug-only thread contract (AUD-115): persistence is synchronous on the
  // GUI thread. Q_ASSERT compiles out in release builds — no runtime guard.
  Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());
  QFile file(getFilePath(key));
  if (!file.open(QIODevice::ReadOnly)) {
    if (file.exists()) {
      qCWarning(appPersistence)
          << "Persistence load failed for key:" << key << "-"
          << toString(PersistenceError::IoError)
          << "(file exists but could not be opened)";
      return PersistenceResult<QJsonObject>::failure(
          PersistenceError::IoError);
    }
    qCDebug(appPersistence)
        << "Persistence load: no state file for key:" << key << "(first run)";
    return PersistenceResult<QJsonObject>::failure(
        PersistenceError::NotFound);
  }

  const QByteArray data = file.readAll();
  QJsonParseError parseError{};
  const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

  if (parseError.error != QJsonParseError::NoError) {
    qCWarning(appPersistence)
        << "Persistence load failed for key:" << key << "-"
        << toString(PersistenceError::InvalidData)
        << "- JSON parse error:" << parseError.errorString();
    return PersistenceResult<QJsonObject>::failure(
        PersistenceError::InvalidData);
  }

  if (!doc.isObject()) {
    qCWarning(appPersistence)
        << "Persistence load failed for key:" << key << "-"
        << toString(PersistenceError::InvalidData)
        << "- JSON is not an object";
    return PersistenceResult<QJsonObject>::failure(
        PersistenceError::InvalidData);
  }

  return PersistenceResult<QJsonObject>::success(doc.object());
}

PersistenceResult<void>
FilePersistenceProvider::saveState(const QString &key,
                                   const QJsonObject &state) {
  // Debug-only thread contract (AUD-115): persistence is synchronous on the
  // GUI thread. Q_ASSERT compiles out in release builds — no runtime guard.
  Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());
  const QString filePath = getFilePath(key);
  QDir dir;
  if (!dir.mkpath(QFileInfo(filePath).absolutePath())) {
    qCWarning(appPersistence)
        << "Persistence save failed for key:" << key << "-"
        << toString(PersistenceError::IoError)
        << "- failed to create data directory";
    return PersistenceResult<void>::failure(PersistenceError::IoError);
  }

  QSaveFile file(filePath);
  if (!file.open(QIODevice::WriteOnly)) {
    qCWarning(appPersistence)
        << "Persistence save failed for key:" << key << "-"
        << toString(PersistenceError::IoError) << "- QSaveFile open failed";
    return PersistenceResult<void>::failure(PersistenceError::IoError);
  }

  const QByteArray data = QJsonDocument(state).toJson(QJsonDocument::Compact);
  if (file.write(data) != data.size()) {
    qCWarning(appPersistence)
        << "Persistence save failed for key:" << key << "-"
        << toString(PersistenceError::IoError) << "- write failed";
    return PersistenceResult<void>::failure(PersistenceError::IoError);
  }

  if (!file.commit()) {
    qCWarning(appPersistence)
        << "Persistence save failed for key:" << key << "-"
        << toString(PersistenceError::CommitError)
        << "- QSaveFile commit failed (atomic replace did not complete)";
    return PersistenceResult<void>::failure(PersistenceError::CommitError);
  }

  return PersistenceResult<void>::success();
}
