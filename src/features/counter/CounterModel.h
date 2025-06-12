#ifndef COUNTERMODEL_H
#define COUNTERMODEL_H
#include "ICounterModel.h"

class CounterModel : public ICounterModel {
  Q_OBJECT
  Q_INTERFACES(ICounterModel)
public:
  explicit CounterModel(QObject *parent = nullptr);
  int value() const override;
  void increment() override;
  QMetaObject::Connection connectValueChanged(QObject *receiver,
                                              const char *member) override;
signals:
  void valueChanged(int newValue);

private:
  int m_value;
};

#endif
