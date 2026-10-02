#include "FilePersistenceProvider.h"
#include "../../logging/logging.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>

FilePersistenceProvider::FilePersistenceProvider(QObject *parent)
    : QObject(parent) {}

static QString getFilePath(const QString &key) {
  return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
         "/" + key + ".json";
}

// Quarantine a corrupt state file so the next save does not silently
// overwrite user data with defaults (B-003). Best-effort rename; failure to
// quarantine is logged and load still returns InvalidData.
static void quarantineCorruptFile(const QString &key, const QString &filePath) {
  const QString quarantinePath = filePath + ".corrupt";
  if (QFile::exists(quarantinePath)) {
    QFile::remove(quarantinePath);
  }
  if (!QFile::rename(filePath, quarantinePath)) {
    qCWarning(appPersistence)
        << "Persistence quarantine failed for key:" << key
        << "- could not rename corrupt file to" << quarantinePath;
  } else {
    qCWarning(appPersistence)
        << "Persistence quarantine: corrupt state for key:" << key
        << "renamed to" << quarantinePath;
  }
}

PersistenceResult<QJsonObject>
FilePersistenceProvider::loadState(const QString &key) {
  const QString filePath = getFilePath(key);
  QByteArray data;
  {
    // Close the QFile before parse/quarantine: Windows cannot rename a file
    // while a handle is open (Linux allows it — platform-sensitive bug).
    QFile file(filePath);
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
    data = file.readAll();
  }

  QJsonParseError parseError{};
  const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

  if (parseError.error != QJsonParseError::NoError) {
    qCWarning(appPersistence)
        << "Persistence load failed for key:" << key << "-"
        << toString(PersistenceError::InvalidData)
        << "- JSON parse error:" << parseError.errorString();
    quarantineCorruptFile(key, filePath);
    return PersistenceResult<QJsonObject>::failure(
        PersistenceError::InvalidData);
  }

  if (!doc.isObject()) {
    qCWarning(appPersistence)
        << "Persistence load failed for key:" << key << "-"
        << toString(PersistenceError::InvalidData)
        << "- JSON is not an object";
    quarantineCorruptFile(key, filePath);
    return PersistenceResult<QJsonObject>::failure(
        PersistenceError::InvalidData);
  }

  return PersistenceResult<QJsonObject>::success(doc.object());
}

PersistenceResult<void>
FilePersistenceProvider::saveState(const QString &key,
                                   const QJsonObject &state) {
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
