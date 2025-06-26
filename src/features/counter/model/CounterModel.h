#pragma once
#include "../../../core/IModel.h"
#include <QMetaMethod>
#include <QObject>
#include <QtPlugin>

class CounterModel : public QObject, public IModel {
  Q_OBJECT

public:
  explicit CounterModel(QObject *parent = nullptr);
  void shutdown() override;
  [[nodiscard]] int value() const;
  void increment();
  void reset();
  QMetaObject::Connection connectValueChanged(QObject *receiver,
                                              const char *member);
signals:
  void valueChanged(int _t1);

private:
  int m_value{0};
  void loadState();
  void saveState() const;
};
