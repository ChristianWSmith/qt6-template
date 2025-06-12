#include "CounterModel.h"
#include <fmt/core.h>

Counter::Counter(QObject *parent) : QObject(parent), m_value(0) {
  fmt::print("Counter model initialized with value: {}\n", m_value);
}

int Counter::value() const { return m_value; }

void Counter::increment() {
  m_value++;
  emit valueChanged(m_value);
  fmt::print("Counter model incremented to: {}\n", m_value);
}