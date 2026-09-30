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
    if (result.error() == PersistenceError::NotFound) {
      return;
    }
    qCDebug(appPersistence)
        << "CounterModel skipped load due to persistence error:"
        << toString(result.error());
    return;
  }
  const QJsonObject &obj = result.value();
  if (obj.contains(KEY_VALUE) && obj.value(KEY_VALUE).isDouble()) {
    m_value = obj.value(KEY_VALUE).toInt();
  }
}

PersistenceResult<void> CounterModel::saveState() const {
  QJsonObject obj;
  obj.insert(KEY_VALUE, m_value);
  // Provider owns error logging; model forwards the result to the caller
  // (composition root observes outcomes at closeEvent).
  return m_provider.saveState(m_key, obj);
}
