#pragma once
#include "../../../core/IModel.h"
#include "../../../core/interfacecommon.h"
#include "../countercommon.h"
#include <QMetaMethod>
#include <QObject>
#include <QtPlugin>

class ICounterModel : public QObject, public IModel {
  Q_OBJECT

public:
  explicit ICounterModel(QObject *parent = nullptr) : QObject(parent) {}
  virtual ~ICounterModel() = default;

  ICounterModel(const ICounterModel &) = delete;
  ICounterModel &operator=(const ICounterModel &) = delete;
  ICounterModel(ICounterModel &&) = delete;
  ICounterModel &operator=(ICounterModel &&) = delete;

  [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] virtual int
  value() const = 0;
  virtual void increment() = 0;
  virtual void reset() = 0;
  virtual QMetaObject::Connection connectValueChanged(QObject *receiver,
                                                      const char *member) = 0;
};

Q_DECLARE_INTERFACE(ICounterModel, COUNTER_FEATURE_ID QINTERFACE_MODEL_SUFFIX)
