#include "CounterModel.h"
#include <fmt/core.h>

CounterModel::CounterModel(QObject *parent)
    : ICounterModel(parent), m_value(0) {
  // log
}

int CounterModel::value() const { return m_value; }

void CounterModel::increment() {
  m_value++;
  emit valueChanged(m_value);
}

QMetaObject::Connection CounterModel::connectValueChanged(QObject *receiver,
                                                          const char *member) {
  return QObject::connect(this, SIGNAL(valueChanged(int)), receiver, member);
}
