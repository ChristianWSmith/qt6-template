#ifndef MESSAGEOUTPUT_H
#define MESSAGEOUTPUT_H

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QMutex>
#include <QMutexLocker>

void messageHandler(QtMsgType type, const QMessageLogContext &context,
                    const QString &msg);

#endif