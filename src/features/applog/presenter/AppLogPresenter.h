#pragma once

#include "../model/AppLogModel.h"
#include "../widget/AppLogWidget.h"

#include "../../../events/LogEvent.h"

#include <QObject>
#include <QPointer>

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
  // Non-owning QPointer refs. Owned via Qt parent-child under AppMainWindow.
  // Slots null-guard before calling into model/widget (dependencies may be
  // destroyed mid-session; QPointer observes destruction and reports null).
  // Presenter destructors must not dereference these pointers.
  // Connections auto-disconnect when either QObject is destroyed.
  QPointer<AppLogModel> m_model;
  QPointer<AppLogWidget> m_view;
};
