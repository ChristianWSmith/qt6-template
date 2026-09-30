#pragma once
#include "../countercommon.h"
#include "../model/CounterModel.h"
#include "../widget/CounterWidget.h"
#include <QObject>

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
  friend class CounterTest;
  // Non-owning. Lifetime is structurally guaranteed by AppMainWindow:
  // model and widget are constructed before the presenter and owned via
  // Qt parent-child. The presenter does not outlive its dependencies.
  CounterModel *m_model;
  CounterWidget *m_view;
};
