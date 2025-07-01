#include "AppLogPresenter.h"
#include <QDebug>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <fmt/core.h>
#include <qlogging.h>

AppLogPresenter::AppLogPresenter(AppLogModel *model, AppLogWidget *view,
                                 QObject *parent)
    : QObject(parent), m_model(model), m_view(view) {

  events::subscribe<LogEvent>(this, &AppLogPresenter::onLogEventReceived);

  if (m_model == nullptr) {
    qWarning() << "AppLogPresenter instantiated without model";
  }
  if (m_view == nullptr) {
    qWarning() << "AppLogPresenter instantiated without view";
  }

  if (m_view != nullptr) {
    connect(m_view, &AppLogWidget::clearRequested, this,
            &AppLogPresenter::handleClearRequested);
  }

  if (m_model != nullptr) {
    connect(m_model, &AppLogModel::logChanged, this,
            &AppLogPresenter::handleLogChanged);
    connect(m_model, &AppLogModel::logCleared, this,
            &AppLogPresenter::handleLogCleared);
  }

  if (m_model != nullptr && m_view != nullptr) {
    m_view->setLogMessages(m_model->getLogMessages());
  }
  qDebug() << "AppLogPresenter instantiated";
}

void AppLogPresenter::onLogEventReceived(const LogEvent &event) {
  qDebug() << "Received log:" << QString::fromStdString(event.message);
  if (m_model != nullptr) {
    m_model->addLogMessage(QString::fromStdString(event.message));
  }
}

void AppLogPresenter::handleLogChanged(const LogDelta &logDelta) {
  if (m_view != nullptr) {
    m_view->handleLogChanged(logDelta);
  }
}

void AppLogPresenter::handleLogCleared() {
  if (m_view != nullptr) {
    m_view->clear();
  }
}

void AppLogPresenter::handleClearRequested() {
  if (m_model != nullptr) {
    m_model->clear();
  }
}

void AppLogPresenter::shutdown() {
  qInfo() << "AppLogPresenter::shutdown()";

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
