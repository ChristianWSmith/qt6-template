#pragma once

#include "../model/AppLogModel.h"
#include "../widget/AppLogWidget.h"

#include "../../../events/DemoLogEvent.h"
#include "../../../events/system/EventSystem.hpp"

#include <QObject>
#include <QPointer>

class AppLogPresenter : public QObject {
  Q_OBJECT

public:
  explicit AppLogPresenter(AppLogModel *model, AppLogWidget *view,
                           QObject *parent = nullptr);

private slots:
  void onDemoLogEventReceived(const DemoLogEvent &event);

  // Full-state view refresh (AUD-116). The LogDelta payload is intentionally
  // ignored — the widget no longer re-encodes the model trim protocol.
  void handleLogChanged();
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

  // EventSystem demo subscription (DemoLogEvent — no production publisher;
  // see AGENTS.md Production status of DemoLogEvent / AppLog). Stored for
  // symmetry with ServiceRegistry's retained Subscription and so reset() is
  // available. Presenter is a QObject child of the window; the connection
  // also auto-disconnects with receiver destruction if the handle is released.
  events::Subscription m_demoLogSubscription;
};
