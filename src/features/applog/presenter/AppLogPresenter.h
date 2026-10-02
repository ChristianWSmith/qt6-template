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
  // Ctor-by-reference encodes required non-null dependencies (matches models
  // and AppMainWindow::constructFeatures). QPointer members observe
  // mid-session destruction; slots null-guard as belt-and-suspenders.
  explicit AppLogPresenter(AppLogModel &model, AppLogWidget &view,
                           QObject *parent = nullptr);

private slots:
  void onDemoLogEventReceived(const DemoLogEvent &event);

  // Full-state view refresh (AUD-116). The LogDelta payload is intentionally
  // ignored — the presenter pushes the complete retained list via
  // setLogMessages; the widget no longer re-encodes the model trim protocol.
  void handleLogChanged();
  void handleLogCleared();
  void handleClearRequested();

private:
  // Non-owning QPointer refs. Owned via Qt parent-child under AppMainWindow.
  // Slots null-guard as belt-and-suspenders (EventSystem path + partial-
  // destruction tests). Presenter destructors must not dereference these.
  QPointer<AppLogModel> m_model;
  QPointer<AppLogWidget> m_view;

  // EventSystem demo subscription (DemoLogEvent — no production publisher;
  // see AGENTS.md Production status of DemoLogEvent / AppLog). Stored for
  // symmetry with ServiceRegistry's retained Subscription and so reset() is
  // available. Presenter is a QObject child of the window; the connection
  // also auto-disconnects with receiver destruction if the handle is released.
  events::Subscription m_demoLogSubscription;
};
