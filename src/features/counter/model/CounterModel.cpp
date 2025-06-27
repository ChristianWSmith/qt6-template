#include "CounterModel.h"
#include "../../../events/LogEvent.h"
#include "../../../events/bus/EventBus.hpp"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <fmt/core.h>

CounterModel::CounterModel(IPersistenceProvider *provider, QObject *parent)
    : QObject(parent), m_provider(provider) {
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

void CounterModel::loadState() {
  if (m_provider == nullptr) {
    return;
  }
  const auto obj = m_provider->loadState(m_key);
  if (obj.contains("value") && obj["value"].isDouble()) {
    m_value = obj["value"].toInt();
  }
}

void CounterModel::saveState() const {
  if (m_provider == nullptr) {
    return;
  }
  QJsonObject obj;
  obj["value"] = m_value;
  m_provider->saveState(m_key, obj);
}

void CounterModel::shutdown() {
  qInfo() << "CounterModel::shutdown()";
  saveState();
}