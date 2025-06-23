#pragma once

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QLoggingCategory>
#include <QMutex>
#include <QMutexLocker>

enum class LogLevel : std::uint8_t { Debug, Info, Warn, Error, None };

void messageHandler(QtMsgType type, const QMessageLogContext &context,
                    const QString &msg);

void setLogLevel(LogLevel logLevel);

LogLevel parseLogLevel(const std::string &levelStr);
