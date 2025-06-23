#pragma once
#include "ICounterModel.h"

class CounterModel : public ICounterModel {
  Q_OBJECT
  Q_INTERFACES(ICounterModel)

public:
  explicit CounterModel(QObject *parent = nullptr);
  void shutdown() override;
  [[nodiscard]] int value() const override;
  void increment() override;
  void reset() override;
  QMetaObject::Connection connectValueChanged(QObject *receiver,
                                              const char *member) override;
signals:
  void valueChanged(int _t1);

private:
  int m_value{0};
  void loadState();
  void saveState() const;
};
