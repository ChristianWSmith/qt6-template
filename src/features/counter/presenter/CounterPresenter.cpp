#include "CounterPresenter.h"
#include "../../../events/LogEvent.hpp"
#include "../../../events/bus/EventBus.hpp"
#include <fmt/core.h>

CounterPresenter::CounterPresenter(ICounterModel *model, ICounterWidget *view,
                                   QObject *parent)
    : ICounterPresenter(parent), m_model(model), m_view(view) {
  if (!m_model) {
    // log
  }
  if (!m_view) {
    // log
  }
  if (m_view) {
    m_view->connectIncrementRequested(this, SLOT(handleIncrementRequest()));
  }
  if (m_model) {
    m_model->connectValueChanged(this, SLOT(handleCounterValueChanged(int)));
  }
  if (m_view && m_model) {
    m_view->displayCounter(m_model->value());
  }
}

void CounterPresenter::handleIncrementRequest() {
  if (m_model) {
    m_model->increment();
  }
}

void CounterPresenter::handleCounterValueChanged(int newValue) {
  events::publish(LogEvent{fmt::format("{}", newValue)});
  if (m_view) {
    m_view->displayCounter(newValue);
  }
}
