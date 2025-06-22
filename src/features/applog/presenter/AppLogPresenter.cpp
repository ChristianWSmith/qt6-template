#include "AppLogPresenter.h"
#include <QDebug>
#include <fmt/core.h>
#include <qlogging.h>

AppLogPresenter::AppLogPresenter(IAppLogModel *model, IAppLogWidget *view,
                                 QObject *parent)
    : IAppLogPresenter(parent), m_model(model), m_view(view),
      m_logEventSubscription(
          events::subscribe<LogEvent>([this](const LogEvent &logEvent) {
            QMetaObject::invokeMethod(this, "onLogEventReceived",
                                      Qt::QueuedConnection,
                                      Q_ARG(LogEvent, logEvent));
          })) {
  if (m_model == nullptr) {
    qWarning() << "AppLogPresenter instantiated without model";
  }
  if (m_view == nullptr) {
    qWarning() << "AppLogPresenter instantiated without view";
  }

  if (m_model != nullptr) {
    m_model->connectLogMessageAdded(this, SLOT(handleLogMessageAdded(QString)));
  }
  qDebug() << "AppLogPresenter instantiated";
}

void AppLogPresenter::onLogEventReceived(const LogEvent &event) {
  if (m_model != nullptr) {
    m_model->addLogMessage(QString::fromStdString(event.message));
  }
}

void AppLogPresenter::handleLogMessageAdded(const QString &message) {
  if (m_view != nullptr) {
    m_view->displayLogMessage(message);
  }
}

void AppLogPresenter::shutdown() {
  qInfo() << "AppLogPresenter::shutdown()";
  if (m_view != nullptr) {
    m_view->shutdown();
  }
  if (m_model != nullptr) {
    m_model->shutdown();
  }
}
