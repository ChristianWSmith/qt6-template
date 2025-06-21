#pragma once
#include "../../featurescommon.h"
#include "../countercommon.h"
#include <QWidget>
#include <QtPlugin>

class ICounterWidget : public QWidget {
  Q_OBJECT

public:
  explicit ICounterWidget(QWidget *parent = nullptr) : QWidget(parent) {}
  virtual ~ICounterWidget() = default;
  virtual void displayCounter(int value) = 0;
  virtual QMetaObject::Connection
  connectIncrementRequested(QObject *receiver, const char *member) = 0;
};

Q_DECLARE_INTERFACE(ICounterWidget, COUNTER_FEATURE_ID FEATURE_WIDGET_SUFFIX)
