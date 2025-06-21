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

  ICounterWidget(const ICounterWidget &) = delete;
  ICounterWidget &operator=(const ICounterWidget &) = delete;
  ICounterWidget(ICounterWidget &&) = delete;
  ICounterWidget &operator=(ICounterWidget &&) = delete;

  virtual void displayCounter(int value) = 0;
  virtual QMetaObject::Connection
  connectIncrementRequested(QObject *receiver, const char *member) = 0;
};

Q_DECLARE_INTERFACE(ICounterWidget, COUNTER_FEATURE_ID FEATURE_WIDGET_SUFFIX)
