#include "AppLogPresenter.h"
#include <QDebug>
#include <fmt/core.h>
#include <qlogging.h>

AppLogPresenter::AppLogPresenter(IAppLogModel *model, IAppLogWidget *view,
                                 QObject *parent)
    : IAppLogPresenter(parent), m_model(model), m_view(view),
      m_logEventSubscription(
          events::subscribe<LogEvent>([this](const LogEvent &e) {
            QMetaObject::invokeMethod(this, "onLogEventReceived",
                                      Qt::QueuedConnection, Q_ARG(LogEvent, e));
          })) {
  if (!m_model) {
    qWarning() << "AppLogPresenter instantiated without model";
  }
  if (!m_view) {
    qWarning() << "AppLogPresenter instantiated without view";
  }

  if (m_model) {
    m_model->connectLogMessageAdded(this, SLOT(handleLogMessageAdded(QString)));
  }
  qDebug() << "AppLogPresenter instantiated";
}

void AppLogPresenter::onLogEventReceived(const LogEvent &event) {
  if (m_model) {
    m_model->addLogMessage(QString::fromStdString(event.message));
  }
}

void AppLogPresenter::handleLogMessageAdded(const QString &message) {
  if (m_view) {
    m_view->displayLogMessage(message);
  }
}
