#pragma once

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QLoggingCategory>
#include <QMutex>
#include <QMutexLocker>

Q_DECLARE_LOGGING_CATEGORY(appMain)
Q_DECLARE_LOGGING_CATEGORY(appFeature)
Q_DECLARE_LOGGING_CATEGORY(appPersistence)
Q_DECLARE_LOGGING_CATEGORY(appEvent)
Q_DECLARE_LOGGING_CATEGORY(appService)

void messageHandler(QtMsgType type, const QMessageLogContext &context,
                    const QString &msg);
