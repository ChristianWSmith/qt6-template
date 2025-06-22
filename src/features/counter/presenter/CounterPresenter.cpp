#include "CounterPresenter.h"
#include "../../../events/LogEvent.h"
#include "../../../events/bus/EventBus.hpp"
#include <fmt/core.h>

CounterPresenter::CounterPresenter(ICounterModel *model, ICounterWidget *view,
                                   QObject *parent)
    : ICounterPresenter(parent), m_model(model), m_view(view) {
  if (m_model == nullptr) {
    qWarning() << "CounterPresenter instantiated without model";
  }
  if (m_view == nullptr) {
    qWarning() << "CounterPresenter instantiated without view";
  }

  if (m_view != nullptr) {
    m_view->connectIncrementRequested(this, SLOT(handleIncrementRequest()));
  }
  if (m_model != nullptr) {
    m_model->connectValueChanged(this, SLOT(handleCounterValueChanged(int)));
  }
  if ((m_view != nullptr) && (m_model != nullptr)) {
    m_view->displayCounter(m_model->value());
  }
  qDebug() << "CounterPresenter instantiated";
}

void CounterPresenter::handleIncrementRequest() {
  events::publish(LogEvent{
      "CounterPresenter::handleIncrementRequest() → m_model->increment()"});
  if (m_model != nullptr) {
    m_model->increment();
  }
}

void CounterPresenter::handleCounterValueChanged(int newValue) {
  events::publish(
      LogEvent{fmt::format("CounterPresenter::handleCounterValueChanged({}) → "
                           "m_view->displayCounter({})",
                           newValue, newValue)});
  if (m_view != nullptr) {
    m_view->displayCounter(newValue);
  }
}

void CounterPresenter::shutdown() {
  qInfo() << "CounterPresenter::shutdown()";
  if (m_view != nullptr) {
    m_view->shutdown();
  }
  if (m_model != nullptr) {
    m_model->shutdown();
  }
}
