#pragma once

#include "../model/AppLogModel.h"
#include "../widget/AppLogWidget.h"

#include "../../../events/DemoLogEvent.h"

#include <QObject>
#include <QPointer>

#include "../applogcommon.h"

class AppLogPresenter : public QObject {
  Q_OBJECT

public:
  // Ctor-by-reference encodes required non-null dependencies (matches models).
  explicit AppLogPresenter(AppLogModel &model, AppLogWidget &view,
                           QObject *parent = nullptr);

private slots:
  void onDemoLogEventReceived(const DemoLogEvent &event);

  void handleLogChanged(const LogDelta &logDelta);
  void handleLogCleared();
  void handleClearRequested();

private:
  // Non-owning QPointer refs. Owned via Qt parent-child under AppMainWindow.
  // Slots null-guard as belt-and-suspenders (EventSystem path + partial-
  // destruction tests). Presenter destructors must not dereference these.
  QPointer<AppLogModel> m_model;
  QPointer<AppLogWidget> m_view;
};
