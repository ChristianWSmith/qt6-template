#pragma once
#include "../../../core/IPresenter.h"
#include "../../../core/interfacecommon.h"
#include "../countercommon.h"
#include <QObject>
#include <QtPlugin>

class ICounterPresenter : public QObject, public IPresenter {
  Q_OBJECT

public:
  explicit ICounterPresenter(QObject *parent = nullptr) : QObject(parent) {}
  virtual ~ICounterPresenter() = default;

  ICounterPresenter(const ICounterPresenter &) = delete;
  ICounterPresenter &operator=(const ICounterPresenter &) = delete;
  ICounterPresenter(ICounterPresenter &&) = delete;
  ICounterPresenter &operator=(ICounterPresenter &&) = delete;
};

Q_DECLARE_INTERFACE(ICounterPresenter,
                    COUNTER_FEATURE_ID QINTERFACE_PRESENTER_SUFFIX)
