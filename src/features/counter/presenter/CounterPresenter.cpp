#include "CounterPresenter.h"
#include "../../../events/LogEvent.h"
#include "../../../events/system/EventSystem.hpp"
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <fmt/core.h>

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

void CounterPresenter::handleResetRequest() {
  events::publish(
      LogEvent{"CounterPresenter::handleResetRequest() → m_model->reset()"});
  if (m_model != nullptr) {
    m_model->reset();
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

  QFuture<void> modelFuture;
  QFuture<void> viewFuture;

  if (m_view != nullptr) {
    modelFuture = QtConcurrent::run([this]() { m_model->shutdown(); });
  }
  if (m_model != nullptr) {
    viewFuture = QtConcurrent::run([this]() { m_view->shutdown(); });
  }

  modelFuture.waitForFinished();
  viewFuture.waitForFinished();
}
