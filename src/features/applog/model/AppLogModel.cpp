#include "AppLogModel.h"
#include <QDateTime>
#include <QDebug>
#include <fmt/core.h>

AppLogModel::AppLogModel(QObject *parent) : IAppLogModel(parent) {
  qDebug() << "AppLogModel instantiated";
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

  emit logChanged(LogChange{timestampedMessage, trimmed});
}

QMetaObject::Connection AppLogModel::connectLogChanged(QObject *receiver,
                                                       const char *member) {
  return QObject::connect(this, SIGNAL(logChanged(LogChange)), receiver,
                          member);
}

void AppLogModel::shutdown() { qInfo() << "AppLogModel::shutdown()"; }
