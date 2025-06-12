#ifndef COUNTERPRESENTER_H
#define COUNTERPRESENTER_H

#include <QObject>

class Counter;
class CounterWidget;

class CounterPresenter : public QObject {
  Q_OBJECT

public:
  explicit CounterPresenter(Counter *model, CounterWidget *view,
                            QObject *parent = nullptr);

private slots:

  void handleIncrementRequest();

  void handleCounterValueChanged(int newValue);

private:
  Counter *m_model;
  CounterWidget *m_view;
};

#endif