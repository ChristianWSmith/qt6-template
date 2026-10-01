#pragma once
#include "../countercommon.h"
#include "../model/CounterModel.h"
#include "../widget/CounterWidget.h"
#include <QObject>
#include <QPointer>

class CounterPresenter : public QObject {
  Q_OBJECT

public:
  explicit CounterPresenter(CounterModel *model, CounterWidget *view,
                            QObject *parent = nullptr);

private slots:
  void handleIncrementRequest();
  void handleResetRequest();
  void handleCounterValueChanged(int newValue);

private:
  // Non-owning QPointer refs. Owned via Qt parent-child under AppMainWindow.
  // Slots null-guard before calling into model/widget (dependencies may be
  // destroyed mid-session; QPointer observes destruction and reports null).
  // Presenter destructors must not dereference these pointers.
  // Connections auto-disconnect when either QObject is destroyed.
  QPointer<CounterModel> m_model;
  QPointer<CounterWidget> m_view;
};
