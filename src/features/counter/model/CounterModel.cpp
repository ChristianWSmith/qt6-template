#include "CounterModel.h"
#include <QDebug>
#include <QJsonObject>

namespace {
constexpr auto KEY_VALUE = "value";
} // namespace

CounterModel::CounterModel(IPersistenceProvider *provider, QObject *parent)
    : QObject(parent), m_provider(provider) {
  qDebug() << "CounterModel instantiated";
  CounterModel::loadState();
}

int CounterModel::value() const { return m_value; }

void CounterModel::increment() {
  m_value++;
  emit valueChanged(m_value);
}

void CounterModel::reset() {
  m_value = 0;
  emit valueChanged(m_value);
}

void CounterModel::loadState() {
  if (m_provider == nullptr) {
    return;
  }
  auto result = m_provider->loadState(m_key);
  if (result.hasError()) {
    if (result.error() != PersistenceError::NotFound) {
      qWarning() << "Failed to load counter state:" << static_cast<int>(result.error());
    }
    return;
  }
  const auto obj = *result;
  if (obj.contains(KEY_VALUE) && obj[KEY_VALUE].isDouble()) {
    m_value = obj[KEY_VALUE].toInt();
  }
}

void CounterModel::saveState() const {
  if (m_provider == nullptr) {
    return;
  }
  QJsonObject obj;
  obj[KEY_VALUE] = m_value;
  auto result = m_provider->saveState(m_key, obj);
  if (result.hasError()) {
    qWarning() << "Failed to save counter state:" << static_cast<int>(result.error());
  }
}
