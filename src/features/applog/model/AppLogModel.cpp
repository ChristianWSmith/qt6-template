#include "AppLogModel.h"
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <fmt/core.h>
#include <unistd.h>

namespace {
QString stateFilePath() {
  return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
         "/applog_model_state.json";
}
} // namespace

AppLogModel::AppLogModel(QObject *parent) : IAppLogModel(parent) {
  qDebug() << "AppLogModel instantiated";
  loadState();
}

const static int MAX_LOG_SIZE = 100;

void AppLogModel::addLogMessage(const QString &message) {

  QString timestampedMessage =
      QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") + " - " +
      message;

  m_logMessages.append(timestampedMessage);

  bool trimmed = false;
  if (m_logMessages.size() > MAX_LOG_SIZE) {
    m_logMessages.removeFirst();
    trimmed = true;
  }

  emit logChanged(LogDelta{timestampedMessage, trimmed});
}

void AppLogModel::clear() {
  m_logMessages.clear();
  emit logCleared();
}

const QVector<QString> &AppLogModel::getLogMessages() const {
  return m_logMessages;
}

QMetaObject::Connection AppLogModel::connectLogChanged(QObject *receiver,
                                                       const char *member) {
  return QObject::connect(this, SIGNAL(logChanged(LogDelta)), receiver, member);
}

QMetaObject::Connection AppLogModel::connectLogCleared(QObject *receiver,
                                                       const char *member) {
  return QObject::connect(this, SIGNAL(logCleared()), receiver, member);
}

void AppLogModel::loadState() {
  QFile file(stateFilePath());
  if (!file.open(QIODevice::ReadOnly)) {
    qWarning() << "Could not open state file for reading:"
               << file.errorString();
    return;
  }

  const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
  if (!doc.isObject()) {
    qWarning() << "Invalid JSON structure in state file";
    return;
  }

  const QJsonObject obj = doc.object();
  const QJsonArray messages = obj.value("logMessages").toArray();

  m_logMessages.clear();
  for (const auto &val : messages) {
    if (val.isString()) {
      m_logMessages.append(val.toString());
    }
  }

  qInfo() << "Loaded" << m_logMessages.size() << "log messages";
}

void AppLogModel::saveState() const {
  QDir().mkpath(
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));

  QFile file(stateFilePath());
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    qWarning() << "Could not open state file for writing:"
               << file.errorString();
    return;
  }

  QJsonArray messages;
  for (const QString &msg : m_logMessages) {
    messages.append(msg);
  }

  QJsonObject obj;
  obj["logMessages"] = messages;

  file.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
  file.flush();
  file.waitForBytesWritten(-1);
  ::fsync(file.handle());

  qInfo() << "Saved" << m_logMessages.size() << "log messages";
}

void AppLogModel::shutdown() {
  qInfo() << "AppLogModel::shutdown()";
  saveState();
}
