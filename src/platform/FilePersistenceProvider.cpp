//---platform/FilePersistenceProvider.cpp---
#include "FilePersistenceProvider.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QStandardPaths>

static QString filePath(const QString &key) {
  return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
         "/" + key + ".json";
}

QJsonObject FilePersistenceProvider::loadState(const QString &key) {
  QFile file(filePath(key));
  if (!file.open(QIODevice::ReadOnly)) {
    qWarning() << "Could not open state file:" << file.errorString();
    return {};
  }

  const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
  return doc.isObject() ? doc.object() : QJsonObject{};
}

void FilePersistenceProvider::saveState(const QString &key,
                                        const QJsonObject &state) {
  QDir().mkpath(QFileInfo(filePath(key)).absolutePath());

  QFile file(filePath(key));
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    qWarning() << "Could not write state file:" << file.errorString();
    return;
  }

  file.write(QJsonDocument(state).toJson(QJsonDocument::Compact));
  flushToDisk(file);
  file.close();
}
