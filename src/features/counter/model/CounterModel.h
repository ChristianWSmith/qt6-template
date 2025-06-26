#pragma once
#include "../../../core/IModel.h"
#include <QMetaMethod>
#include <QObject>
#include <QtPlugin>

class CounterModel : public QObject, public IModel {
  Q_OBJECT

public:
  explicit CounterModel(QObject *parent = nullptr);

  virtual void shutdown() override;

  [[nodiscard]] virtual int value() const;
  virtual void increment();
  virtual void reset();

signals:
  void valueChanged(int _t1);

private:
  int m_value{0};
  void loadState();
  void saveState() const;
};
