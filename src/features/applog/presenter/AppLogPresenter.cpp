#include "AppLogPresenter.h"
#include <QDebug>
#include <fmt/core.h>

AppLogPresenter::AppLogPresenter(IAppLogModel *model, IAppLogWidget *view,
                                 QObject *parent)
    : IAppLogPresenter(parent), m_model(model), m_view(view),
      m_logEventSubscription(
          events::subscribe<LogEvent>([this](const LogEvent &e) {
            QMetaObject::invokeMethod(this, "onLogEventReceived",
                                      Qt::QueuedConnection, Q_ARG(LogEvent, e));
          })) {
  if (!m_model) {
    fmt::print(stderr, "Error: AppLogPresenter received null IAppLogModel.\n");
  }
  if (!m_view) {
    fmt::print(stderr, "Error: AppLogPresenter received null IAppLogWidget.\n");
  }

  if (m_model) {
    m_model->connectLogMessageAdded(this, SLOT(handleLogMessageAdded(QString)));
    fmt::print("Presenter: Connected logMessageAdded signal to slot via "
               "interface method.\n");
  }

  qDebug() << "AppLogPresenter initialized and subscribed to LogEvents.";
}

void AppLogPresenter::onLogEventReceived(const LogEvent &event) {
  qDebug() << "AppLogPresenter received LogEvent: "
           << QString::fromStdString(event.message);
  if (m_model) {
    m_model->addLogMessage(QString::fromStdString(event.message));
  }
}

void AppLogPresenter::handleLogMessageAdded(const QString &message) {
  qDebug() << "AppLogPresenter: handleLogMessageAdded called via signal.";
  if (m_view) {
    m_view->displayLogMessage(message);
    fmt::print("Presenter: Displayed log message in view.\n");
  }
}
