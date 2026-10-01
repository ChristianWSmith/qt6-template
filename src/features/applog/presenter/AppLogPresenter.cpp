#include "AppLogPresenter.h"
#include "../../../events/system/EventSystem.hpp"
#include "../../../logging/logging.h"

AppLogPresenter::AppLogPresenter(AppLogModel *model, AppLogWidget *view,
                                 QObject *parent)
    : QObject(parent), m_model(model), m_view(view) {
  Q_ASSERT(m_model != nullptr);
  Q_ASSERT(m_view != nullptr);

  // Demonstration only — stock app never publishes LogEvent in production
  // (see AGENTS.md Production status of LogEvent / AppLog).
  events::subscribe<LogEvent>(this, &AppLogPresenter::onLogEventReceived);

  connect(m_view, &AppLogWidget::clearRequested, this,
          &AppLogPresenter::handleClearRequested);

  connect(m_model, &AppLogModel::logChanged, this,
          &AppLogPresenter::handleLogChanged);
  connect(m_model, &AppLogModel::logCleared, this,
          &AppLogPresenter::handleLogCleared);

  m_view->setLogMessages(m_model->getLogMessages());
  qCDebug(appFeature) << "AppLogPresenter instantiated";
}

void AppLogPresenter::onLogEventReceived(const LogEvent &event) {
  m_model->addLogMessage(QString::fromStdString(event.message));
}

void AppLogPresenter::handleLogChanged(const LogDelta &logDelta) {
  m_view->handleLogChanged(logDelta);
}

void AppLogPresenter::handleLogCleared() {
  m_view->clear();
}

void AppLogPresenter::handleClearRequested() {
  m_model->clear();
}
