#include "AppLogPresenter.h"
#include "../../../events/system/EventSystem.hpp"
#include "../../../logging/logging.h"

AppLogPresenter::AppLogPresenter(AppLogModel *model, AppLogWidget *view,
                                 QObject *parent)
    : QObject(parent), m_model(model), m_view(view) {
  Q_ASSERT(m_model != nullptr);
  Q_ASSERT(m_view != nullptr);

  // Demonstration only — stock app never publishes DemoLogEvent in production
  // (see AGENTS.md Production status of DemoLogEvent / AppLog). Features use Qt
  // signals for real data; this subscription is EventSystem demo wiring only.
  events::subscribe<DemoLogEvent>(this,
                                  &AppLogPresenter::onDemoLogEventReceived);

  connect(m_view, &AppLogWidget::clearRequested, this,
          &AppLogPresenter::handleClearRequested);

  connect(m_model, &AppLogModel::logChanged, this,
          &AppLogPresenter::handleLogChanged);
  connect(m_model, &AppLogModel::logCleared, this,
          &AppLogPresenter::handleLogCleared);

  m_view->setLogMessages(m_model->getLogMessages());
  qCDebug(appFeature) << "AppLogPresenter instantiated";
}

void AppLogPresenter::onDemoLogEventReceived(const DemoLogEvent &event) {
  if (!m_model)
    return;
  m_model->addLogMessage(QString::fromStdString(event.message));
}

void AppLogPresenter::handleLogChanged(const LogDelta &logDelta) {
  if (!m_view)
    return;
  m_view->handleLogChanged(logDelta);
}

void AppLogPresenter::handleLogCleared() {
  if (!m_view)
    return;
  m_view->clear();
}

void AppLogPresenter::handleClearRequested() {
  if (!m_model)
    return;
  m_model->clear();
}
