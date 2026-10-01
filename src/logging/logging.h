#pragma once

#include <QLoggingCategory>
#include <QString>

#include <string>

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

/// Map a CLI log-level token to QLoggingCategory filter rules.
/// Owned by the logging module (not main.cpp). Unknown tokens leave
/// Qt's current rules unchanged and emit qCWarning(appMain).
void configureLogLevel(const std::string &levelStr);
