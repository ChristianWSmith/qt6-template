#include "FilePersistenceProvider.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QStandardPaths>

FilePersistenceProvider::FilePersistenceProvider(QObject *parent)
    : QObject(parent) {}

static QString filePath(const QString &key) {
  return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
         "/" + key + ".json";
}

PersistenceResult<QJsonObject>
FilePersistenceProvider::loadState(const QString &key) {
  QFile file(filePath(key));
  if (!file.open(QIODevice::ReadOnly)) {
    if (file.exists()) {
      return PersistenceResult<QJsonObject>::failure(
          PersistenceError::IoError);
    }
    return PersistenceResult<QJsonObject>::failure(
        PersistenceError::NotFound);
  }

  const QByteArray data = file.readAll();
  QJsonParseError parseError{};
  const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

  if (parseError.error != QJsonParseError::NoError) {
    qWarning() << "JSON parse error:" << parseError.errorString();
    return PersistenceResult<QJsonObject>::failure(
        PersistenceError::InvalidData);
  }

  if (!doc.isObject()) {
    return PersistenceResult<QJsonObject>::failure(
        PersistenceError::InvalidData);
  }

  return PersistenceResult<QJsonObject>::success(doc.object());
}

PersistenceResult<void>
FilePersistenceProvider::saveState(const QString &key,
                                   const QJsonObject &state) {
  const QString destPath = filePath(key);
  const QString dirPath = QFileInfo(destPath).absolutePath();

  if (!QDir().mkpath(dirPath)) {
    qWarning() << "Could not create directory:" << dirPath;
    return PersistenceResult<void>::failure(PersistenceError::IoError);
  }

  const QString tempPath = destPath + ".tmp";

  QFile file(tempPath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    qWarning() << "Could not write state file:" << file.errorString();
    return PersistenceResult<void>::failure(PersistenceError::IoError);
  }

  const QByteArray data =
      QJsonDocument(state).toJson(QJsonDocument::Compact);

  if (file.write(data) != data.size()) {
    qWarning() << "Incomplete write to state file";
    file.remove();
    return PersistenceResult<void>::failure(PersistenceError::IoError);
  }

  if (!flushToDisk(file)) {
    qWarning() << "Flush/fsync failed for state file";
    file.remove();
    return PersistenceResult<void>::failure(PersistenceError::DurabilityFailure);
  }

  file.close();

  if (!file.rename(destPath)) {
    qWarning() << "Could not rename temp file to destination:"
               << file.errorString();
    file.remove();
    return PersistenceResult<void>::failure(PersistenceError::IoError);
  }

  return PersistenceResult<void>::success();
}
