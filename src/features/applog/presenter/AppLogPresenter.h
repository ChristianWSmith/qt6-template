#pragma once

#include "../model/AppLogModel.h"
#include "../widget/AppLogWidget.h"

#include "../../../events/LogEvent.h"

#include <QObject>

#include "../applogcommon.h"

class AppLogPresenter : public QObject {
  Q_OBJECT

public:
  explicit AppLogPresenter(AppLogModel *model, AppLogWidget *view,
                           QObject *parent = nullptr);

private slots:
  void onLogEventReceived(const LogEvent &event);

  void handleLogChanged(const LogDelta &logDelta);
  void handleLogCleared();
  void handleClearRequested();

private:
  friend class AppLogTest;
  // Non-owning. Lifetime is structurally guaranteed by AppMainWindow:
  // model and widget are constructed before the presenter and owned via
  // Qt parent-child. The presenter does not outlive its dependencies.
  AppLogModel *m_model;
  AppLogWidget *m_view;
};
