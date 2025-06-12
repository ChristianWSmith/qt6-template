#ifndef ICOUNTERMODEL_H
#define ICOUNTERMODEL_H
#include "../features_common.h"
#include "counter_common.h"
#include <QMetaMethod>
#include <QObject>
#include <QtPlugin>

class ICounterModel : public QObject {
  Q_OBJECT
public:
  explicit ICounterModel(QObject *parent = nullptr) : QObject(parent) {}
  virtual ~ICounterModel() = default;
  virtual int value() const = 0;
  virtual void increment() = 0;
  virtual QMetaObject::Connection connectValueChanged(QObject *receiver,
                                                      const char *member) = 0;
};

Q_DECLARE_INTERFACE(ICounterModel, COUNTER_FEATURE_ID FEATURE_MODEL_SUFFIX)

#endif
