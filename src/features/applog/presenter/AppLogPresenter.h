#pragma once

#include "../model/IAppLogModel.h"
#include "../widget/IAppLogWidget.h"
#include "IAppLogPresenter.h"

#include "../../../events/LogEvent.h"
#include "../../../events/bus/EventBus.hpp"

#include <QObject>
#include <QString>

class AppLogPresenter : public IAppLogPresenter {
  Q_OBJECT
  Q_INTERFACES(IAppLogPresenter)

public:
  explicit AppLogPresenter(IAppLogModel *model, IAppLogWidget *view,
                           QObject *parent = nullptr);
  void shutdown() override;

private slots:
  void onLogEventReceived(const LogEvent &event);

  void handleLogMessageAdded(const QString &message);

private:
  IAppLogModel *m_model;
  IAppLogWidget *m_view;
  events::Subscription<LogEvent> m_logEventSubscription;
};
