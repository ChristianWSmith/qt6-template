#include "AppLogModel.h"
#include "logging/logging.h"
#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>

namespace {
constexpr int MAX_LOG_SIZE = 100;
constexpr auto KEY_LOG_MESSAGES = "logMessages";
} // namespace

AppLogModel::AppLogModel(IPersistenceProvider &provider, QObject *parent)
    : QObject(parent), m_provider(provider) {
  qCDebug(appPersistence) << "AppLogModel instantiated";
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
  auto result = m_provider.loadState(m_key);
  if (result.hasError()) {
    if (result.error() != PersistenceError::NotFound) {
      qCWarning(appPersistence) << "Failed to load applog state:" << toString(result.error());
    }
    return;
  }

  const QJsonObject &obj = result.value();
  const QJsonArray messages = obj.value(KEY_LOG_MESSAGES).toArray();

  m_logMessages.clear();
  for (const auto &val : messages) {
    if (val.isString()) {
      m_logMessages.append(val.toString());
    }
  }

  qCInfo(appPersistence) << "Loaded" << m_logMessages.size() << "log messages";
}

void AppLogModel::saveState() const {
  QJsonArray messages;
  for (const QString &msg : m_logMessages) {
    messages.append(msg);
  }

  QJsonObject obj;
  obj.insert(KEY_LOG_MESSAGES, messages);

  auto result = m_provider.saveState(m_key, obj);
  if (result.hasError()) {
    qCWarning(appPersistence) << "Failed to save applog state:" << toString(result.error());
  } else {
    qCInfo(appPersistence) << "Saved" << m_logMessages.size() << "log messages";
  }
}
