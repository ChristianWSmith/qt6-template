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

// F-11: custom handler retained (see logging.cpp for full rationale).
// Demonstrates qInstallMessageHandler + fmt; explicit fatal abort;
// stdout/stderr sink split. Canonical alternative if formatting-only is
// enough: qSetMessagePattern + default Qt handler. Categories +
// configureLogLevel remain the canonical filtering layer either way.
void messageHandler(QtMsgType type, const QMessageLogContext &context,
                    const QString &msg);
