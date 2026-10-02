#include "AppLogModel.h"
#include "logging/logging.h"
#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>

namespace {
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
  if (m_logMessages.size() > kMaxLogSize) {
    m_logMessages.removeFirst();
    trimmed = true;
  }

  emit logChanged(LogDelta{.message = timestampedMessage, .trimmed = trimmed});
}

void AppLogModel::clear() {
  m_logMessages.clear();
  emit logCleared();
}

const QStringList &AppLogModel::getLogMessages() const {
  return m_logMessages;
}

void AppLogModel::loadState() {
  auto result = m_provider.loadState(m_key);
  if (result.hasError()) {
    if (result.error() == PersistenceError::NotFound) {
      return;
    }
    // Operational errors are logged by the provider; model only branches.
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

  // Re-apply the retention cap after load so persisted state never
  // exceeds kMaxLogSize even if the file was written by a prior version
  // or an external writer.
  while (m_logMessages.size() > kMaxLogSize) {
    m_logMessages.removeFirst();
  }

  qCInfo(appPersistence) << "Loaded" << m_logMessages.size() << "log messages";
}

PersistenceResult<void> AppLogModel::saveState() const {
  QJsonArray messages;
  for (const QString &msg : m_logMessages) {
    messages.append(msg);
  }

  QJsonObject obj;
  obj.insert(KEY_LOG_MESSAGES, messages);

  // Provider owns error logging; model forwards the result to the caller
  // (composition root observes outcomes at closeEvent).
  return m_provider.saveState(m_key, obj);
}
