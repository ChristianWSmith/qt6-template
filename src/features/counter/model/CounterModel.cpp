#include "CounterModel.h"
#include "logging/logging.h"
#include <QJsonObject>

namespace {
constexpr auto KEY_VALUE = "value";
} // namespace

CounterModel::CounterModel(IPersistenceProvider &provider, QObject *parent)
    : QObject(parent), m_provider(provider) {
  qCDebug(appPersistence) << "CounterModel instantiated";
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
  auto result = m_provider.loadState(m_key);
  if (result.hasError()) {
    if (result.error() != PersistenceError::NotFound) {
      qCWarning(appPersistence) << "Failed to load counter state:" << toString(result.error());
    }
    return;
  }
  const auto obj = result.value();
  if (obj.contains(KEY_VALUE) && obj[KEY_VALUE].isDouble()) {
    m_value = obj[KEY_VALUE].toInt();
  }
}

void CounterModel::saveState() const {
  QJsonObject obj;
  obj[KEY_VALUE] = m_value;
  auto result = m_provider.saveState(m_key, obj);
  if (result.hasError()) {
    qCWarning(appPersistence) << "Failed to save counter state:" << toString(result.error());
  }
}
