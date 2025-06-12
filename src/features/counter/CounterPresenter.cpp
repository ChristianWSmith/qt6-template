#include "CounterPresenter.h"
#include "CounterModel.h"
#include "CounterWidget.h"
#include <fmt/core.h>

CounterPresenter::CounterPresenter(Counter *model, CounterWidget *view,
                                   QObject *parent)
    : QObject(parent), m_model(model), m_view(view) {

  connect(m_view, &CounterWidget::incrementRequested, this,
          &CounterPresenter::handleIncrementRequest);

  connect(m_model, &Counter::valueChanged, this,
          &CounterPresenter::handleCounterValueChanged);

  m_view->displayCounter(m_model->value());
  fmt::print("Presenter initialized. Initial counter displayed: {}\n",
             m_model->value());
}

void CounterPresenter::handleIncrementRequest() { m_model->increment(); }

void CounterPresenter::handleCounterValueChanged(int newValue) {

  m_view->displayCounter(newValue);
  fmt::print("Presenter updated view to: {}\n", newValue);
}