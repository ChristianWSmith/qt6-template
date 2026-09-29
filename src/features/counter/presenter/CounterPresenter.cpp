#include "CounterPresenter.h"

CounterPresenter::CounterPresenter(CounterModel *model, CounterWidget *view,
                                   QObject *parent)
    : QObject(parent), m_model(model), m_view(view) {
  if (m_model == nullptr) {
    qWarning() << "CounterPresenter instantiated without model";
  }
  if (m_view == nullptr) {
    qWarning() << "CounterPresenter instantiated without view";
  }

  if (m_view != nullptr) {
    connect(m_view, &CounterWidget::resetRequested, this,
            &CounterPresenter::handleResetRequest);
    connect(m_view, &CounterWidget::incrementRequested, this,
            &CounterPresenter::handleIncrementRequest);
  }

  if (m_model != nullptr) {
    connect(m_model, &CounterModel::valueChanged, this,
            &CounterPresenter::handleCounterValueChanged);
  }
  if (m_view != nullptr) {
    m_view->displayCounter(m_model->value());
  }
  qDebug() << "CounterPresenter instantiated";
}

void CounterPresenter::handleIncrementRequest() {
  if (m_model != nullptr) {
    m_model->increment();
  }
}

void CounterPresenter::handleResetRequest() {
  if (m_model != nullptr) {
    m_model->reset();
  }
}

void CounterPresenter::handleCounterValueChanged(int newValue) {
  if (m_view != nullptr) {
    m_view->displayCounter(newValue);
  }
}
