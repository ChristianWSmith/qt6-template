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
  // Non-owning pointers. Owned via Qt parent-child under AppMainWindow.
  // Presenter destructors must not dereference these pointers.
  // Connections auto-disconnect when either QObject is destroyed.
  AppLogModel *m_model;
  AppLogWidget *m_view;
};
