#include "AppLogModel.h"
#include <QDateTime>
#include <QDebug>
#include <fmt/core.h>

AppLogModel::AppLogModel(QObject *parent) : IAppLogModel(parent) {
  fmt::print("AppLogModel initialized.\n");
}

void AppLogModel::addLogMessage(const QString &message) {

  QString timestampedMessage =
      QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") + " - " +
      message;

  m_logMessages.append(timestampedMessage);

  if (m_logMessages.size() > m_maxLogMessages) {
    m_logMessages.removeFirst();
  }

  fmt::print("Model added message: {}\n", timestampedMessage.toStdString());

  emit logMessageAdded(timestampedMessage);
}

QMetaObject::Connection
AppLogModel::connectLogMessageAdded(QObject *receiver, const char *member) {
  return QObject::connect(this, SIGNAL(logMessageAdded(QString)), receiver,
                          member);
}
