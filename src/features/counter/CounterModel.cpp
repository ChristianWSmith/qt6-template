#include "CounterModel.h"
#include <fmt/core.h>

CounterModel::CounterModel(QObject *parent)
    : ICounterModel(parent), m_value(0) {
  fmt::print("Counter model initialized with value: {}\n", m_value);
}

int CounterModel::value() const { return m_value; }

void CounterModel::increment() {
  m_value++;
  fmt::print("Counter model incremented to: {}\n", m_value);
  emit valueChanged(m_value);
  fmt::print("Counter model: Emitted valueChanged signal.\n");
}

QMetaObject::Connection CounterModel::connectValueChanged(QObject *receiver,
                                                          const char *member) {
  return QObject::connect(this, SIGNAL(valueChanged(int)), receiver, member);
}
