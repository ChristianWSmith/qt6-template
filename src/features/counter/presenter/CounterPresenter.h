#pragma once
#include "../model/ICounterModel.h"
#include "../widget/ICounterWidget.h"
#include "ICounterPresenter.h"
#include <QObject>

class CounterPresenter : public ICounterPresenter {
  Q_OBJECT
  Q_INTERFACES(ICounterPresenter)

public:
  explicit CounterPresenter(ICounterModel *model, ICounterWidget *view,
                            QObject *parent = nullptr);
  void shutdown() override;

private slots:
  void handleIncrementRequest();
  void handleResetRequest();
  void handleCounterValueChanged(int newValue);

private:
  ICounterModel *m_model;
  ICounterWidget *m_view;
};
