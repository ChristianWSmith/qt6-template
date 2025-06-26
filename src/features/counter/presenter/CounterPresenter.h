#pragma once
#include "../../../core/IPresenter.h"
#include "../countercommon.h"
#include "../model/CounterModel.h"
#include "../widget/CounterWidget.h"
#include "CounterPresenter.h"
#include <QObject>
#include <QtPlugin>

class CounterPresenter : public QObject, public IPresenter {
  Q_OBJECT

public:
  explicit CounterPresenter(CounterModel *model, CounterWidget *view,
                            QObject *parent = nullptr);
  void shutdown() override;

private slots:
  void handleIncrementRequest();
  void handleResetRequest();
  void handleCounterValueChanged(int newValue);

private:
  CounterModel *m_model;
  CounterWidget *m_view;
};
