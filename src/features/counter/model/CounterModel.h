#pragma once
#include "ICounterModel.h"

class CounterModel : public ICounterModel {
  Q_OBJECT
  Q_INTERFACES(ICounterModel)

public:
  explicit CounterModel(QObject *parent = nullptr);
  [[nodiscard]] int value() const override;
  void increment() override;
  QMetaObject::Connection connectValueChanged(QObject *receiver,
                                              const char *member) override;
signals:
  void valueChanged(int _t1);

private:
  int m_value{0};
};
