#include "CounterPresenter.h"
#include "../../../logging/logging.h"

CounterPresenter::CounterPresenter(CounterModel *model, CounterWidget *view,
                                   QObject *parent)
    : QObject(parent), m_model(model), m_view(view) {
  Q_ASSERT(m_model != nullptr);
  Q_ASSERT(m_view != nullptr);

  connect(m_view, &CounterWidget::resetRequested, this,
          &CounterPresenter::handleResetRequest);
  connect(m_view, &CounterWidget::incrementRequested, this,
          &CounterPresenter::handleIncrementRequest);

  connect(m_model, &CounterModel::valueChanged, this,
          &CounterPresenter::handleCounterValueChanged);

  m_view->displayCounter(m_model->value());
  qCDebug(appFeature) << "CounterPresenter instantiated";
}

void CounterPresenter::handleIncrementRequest() {
  if (!m_model)
    return;
  m_model->increment();
}

void CounterPresenter::handleResetRequest() {
  if (!m_model)
    return;
  m_model->reset();
}

void CounterPresenter::handleCounterValueChanged(int newValue) {
  if (!m_view)
    return;
  m_view->displayCounter(newValue);
}
