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

PersistenceResult<QJsonObject>
FilePersistenceProvider::loadState(const QString &key) {
  QFile file(getFilePath(key));
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
    qCWarning(appPersistence) << "JSON parse error:" << parseError.errorString();
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
  const QString filePath = getFilePath(key);
  QDir dir;
  if (!dir.mkpath(QFileInfo(filePath).absolutePath())) {
    return PersistenceResult<void>::failure(PersistenceError::IoError);
  }

  QSaveFile file(filePath);
  if (!file.open(QIODevice::WriteOnly)) {
    return PersistenceResult<void>::failure(PersistenceError::IoError);
  }

  const QByteArray data = QJsonDocument(state).toJson(QJsonDocument::Compact);
  if (file.write(data) != data.size()) {
    return PersistenceResult<void>::failure(PersistenceError::IoError);
  }

  if (!file.commit()) {
    return PersistenceResult<void>::failure(PersistenceError::DurabilityFailure);
  }

  return PersistenceResult<void>::success();
}
