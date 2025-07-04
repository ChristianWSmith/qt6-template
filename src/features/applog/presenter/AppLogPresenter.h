#pragma once

#include "../model/AppLogModel.h"
#include "../widget/AppLogWidget.h"

#include "../../../events/LogEvent.h"

#include <QObject>
#include <QString>

#include "../../../core/IPresenter.h"
#include "../applogcommon.h"
#include <QtPlugin>

class AppLogPresenter : public QObject, public IPresenter {
  Q_OBJECT

public:
  explicit AppLogPresenter(AppLogModel *model, AppLogWidget *view,
                           QObject *parent = nullptr);

  void shutdown() override;

private slots:
  void onLogEventReceived(const LogEvent &event);

  void handleLogChanged(const LogDelta &logDelta);
  void handleLogCleared();
  void handleClearRequested();

private:
  friend class AppLogTest;
  AppLogModel *m_model;
  AppLogWidget *m_view;
};
