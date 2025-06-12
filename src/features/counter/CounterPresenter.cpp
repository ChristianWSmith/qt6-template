#include "CounterPresenter.h"
#include <fmt/core.h>

CounterPresenter::CounterPresenter(ICounterModel *model, ICounterWidget *view,
                                   QObject *parent)
    : ICounterPresenter(parent), m_model(model), m_view(view) {
  if (!m_model) {
    fmt::print(stderr,
               "Error: CounterPresenter received null ICounterModel.\n");
  }
  if (!m_view) {
    fmt::print(stderr,
               "Error: CounterPresenter received null ICounterWidget.\n");
  }
  if (m_view) {
    m_view->connectIncrementRequested(this, SLOT(handleIncrementRequest()));
    fmt::print("Presenter: Connected incrementRequested signal to slot via "
               "interface method.\n");
  }
  if (m_model) {
    m_model->connectValueChanged(this, SLOT(handleCounterValueChanged(int)));
    fmt::print("Presenter: Connected valueChanged signal to slot via interface "
               "method.\n");
  }
  if (m_view && m_model) {
    m_view->displayCounter(m_model->value());
    fmt::print("Presenter initialized. Initial counter displayed: {}\n",
               m_model->value());
  }
}

void CounterPresenter::handleIncrementRequest() {
  fmt::print("Presenter: handleIncrementRequest called via signal. "
             "Incrementing model.\n");
  if (m_model) {
    m_model->increment();
  }
}

void CounterPresenter::handleCounterValueChanged(int newValue) {
  fmt::print("Presenter: handleCounterValueChanged called via signal.\n");
  if (m_view) {
    m_view->displayCounter(newValue);
    fmt::print("Presenter: Updated view to: {}\n", newValue);
  }
}
