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
  CounterModel *m_model;
  CounterWidget *m_view;
};
