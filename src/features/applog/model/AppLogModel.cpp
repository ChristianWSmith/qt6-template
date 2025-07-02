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

namespace {
const int MAX_LOG_SIZE = 100;
constexpr auto KEY_LOG_MESSAGES = "logMessages";
} // namespace

AppLogModel::AppLogModel(IPersistenceProvider *provider, QObject *parent)
    : QObject(parent), m_provider(provider) {
  qDebug() << "AppLogModel instantiated";
  AppLogModel::loadState();
}

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

  emit logChanged(LogDelta{.message = timestampedMessage, .trimmed = trimmed});
}

void AppLogModel::clear() {
  m_logMessages.clear();
  emit logCleared();
}

const QVector<QString> &AppLogModel::getLogMessages() const {
  return m_logMessages;
}

void AppLogModel::loadState() {
  if (m_provider == nullptr) {
    return;
  }

  QJsonObject obj = m_provider->loadState(m_key);
  const QJsonArray messages = obj.value(KEY_LOG_MESSAGES).toArray();

  m_logMessages.clear();
  for (const auto &val : messages) {
    if (val.isString()) {
      m_logMessages.append(val.toString());
    }
  }

  qInfo() << "Loaded" << m_logMessages.size() << "log messages";
}

void AppLogModel::saveState() const {
  if (m_provider == nullptr) {
    return;
  }

  QJsonArray messages;
  for (const QString &msg : m_logMessages) {
    messages.append(msg);
  }

  QJsonObject obj;
  obj[KEY_LOG_MESSAGES] = messages;

  m_provider->saveState(m_key, obj);
  qInfo() << "Saved" << m_logMessages.size() << "log messages";
}

void AppLogModel::shutdown() {
  qInfo() << "AppLogModel::shutdown()";
  saveState();
}
