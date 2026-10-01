#pragma once
#include "../../../core/IModel.h"
#include "../../../core/IPersistenceProvider.h"
#include "../countercommon.h"
#include <QObject>

class CounterModel : public QObject, public IModel {
  Q_OBJECT

public:
  explicit CounterModel(IPersistenceProvider &provider,
                        QObject *parent = nullptr);

  // IModel
  PersistenceResult<void> saveState() const override;
  void loadState() override;

  [[nodiscard]] int value() const;
  void increment();
  void reset();

signals:
  void valueChanged(int _t1);

private:
  IPersistenceProvider &m_provider;
  const QString m_key{APP_ID ".CounterState"};

  int m_value{0};
};
