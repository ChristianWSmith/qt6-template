#include "CounterModel.h"
#include "../../../events/LogEvent.h"
#include "../../../events/bus/EventBus.hpp"
#include <fmt/core.h>

CounterModel::CounterModel(QObject *parent) : ICounterModel(parent) {
  qDebug() << "CounterModel instantiated";
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

QMetaObject::Connection CounterModel::connectValueChanged(QObject *receiver,
                                                          const char *member) {
  return QObject::connect(this, SIGNAL(valueChanged(int)), receiver, member);
}

void CounterModel::shutdown() { qInfo() << "CounterModel::shutdown()"; }