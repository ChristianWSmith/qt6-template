#pragma once
#include "../../IPresenter.h"
#include "../../featurescommon.h"
#include "../countercommon.h"
#include <QObject>
#include <QtPlugin>

class ICounterPresenter : public QObject, public IPresenter {
  Q_OBJECT

public:
  explicit ICounterPresenter(QObject *parent = nullptr) : QObject(parent) {}
  virtual ~ICounterPresenter() = default;
};

Q_DECLARE_INTERFACE(ICounterPresenter,
                    COUNTER_FEATURE_ID FEATURE_PRESENTER_SUFFIX)
