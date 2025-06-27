#pragma once
#include "../../../core/IModel.h"
#include "../../../core/IPersistenceProvider.h"
#include "../countercommon.h"
#include <QMetaMethod>
#include <QObject>
#include <QtPlugin>

class CounterModel : public QObject, public IModel {
  Q_OBJECT

public:
  explicit CounterModel(IPersistenceProvider *provider = nullptr,
                        QObject *parent = nullptr);

  void shutdown() override;

  [[nodiscard]] int value() const;
  void increment();
  void reset();

signals:
  void valueChanged(int _t1);

private:
  friend class CounterTest;

  IPersistenceProvider *m_provider;
  const QString m_key{APP_ID ".CounterState"};

  void saveState() const override;
  void loadState() override;

  int m_value{0};
};
