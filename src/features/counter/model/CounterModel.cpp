#include "CounterModel.h"
#include "../../../events/LogEvent.h"
#include "../../../events/bus/EventBus.hpp"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <fmt/core.h>
#include <unistd.h>

namespace {
QString stateFilePath() {
  return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
         "/counter_model_state.json";
}
} // namespace

CounterModel::CounterModel(QObject *parent) : ICounterModel(parent) {
  qDebug() << "CounterModel instantiated";
  loadState();
}

int CounterModel::value() const {
  events::publish(LogEvent{fmt::format("CounterModel::value() → {}", m_value)});
  return m_value;
}

void CounterModel::increment() {
  events::publish(LogEvent{fmt::format(
      "CounterModel::increment() → emit valueChanged({})", m_value + 1)});
  m_value++;
  emit valueChanged(m_value);
}

void CounterModel::reset() {
  events::publish(LogEvent{"CounterModel::reset() → emit valueChanged(0)"});
  m_value = 0;
  emit valueChanged(m_value);
}

QMetaObject::Connection CounterModel::connectValueChanged(QObject *receiver,
                                                          const char *member) {
  return QObject::connect(this, SIGNAL(valueChanged(int)), receiver, member);
}

void CounterModel::loadState() {
  QFile file(stateFilePath());
  if (!file.open(QIODevice::ReadOnly)) {
    qWarning() << "Could not open state file for reading:"
               << file.errorString();
    return;
  }

  const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
  if (!doc.isObject()) {
    qWarning() << "Invalid JSON structure in state file";
    return;
  }

  const QJsonObject obj = doc.object();
  if (obj.contains("value") && obj["value"].isDouble()) {
    m_value = obj["value"].toInt();
    qDebug() << "Loaded counter value:" << m_value;
  }
}

void CounterModel::saveState() const {
  QDir().mkpath(
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));

  QFile file(stateFilePath());
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    qWarning() << "Could not open state file for writing:"
               << file.errorString();
    return;
  }

  QJsonObject obj;
  obj["value"] = m_value;
  QJsonDocument doc(obj);
  file.write(doc.toJson(QJsonDocument::Compact));
  file.flush();
  file.waitForBytesWritten(-1);
  ::fsync(file.handle());

  qDebug() << "Saved counter value:" << m_value;
}

void CounterModel::shutdown() {
  qInfo() << "CounterModel::shutdown()";
  saveState();
}