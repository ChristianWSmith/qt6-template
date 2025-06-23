#pragma once

#include "../../IPresenter.h"
#include "../../featurescommon.h"
#include "../applogcommon.h"
#include <QObject>
#include <QtPlugin>

class IAppLogPresenter : public QObject, public IPresenter {
  Q_OBJECT

public:
  explicit IAppLogPresenter(QObject *parent = nullptr) : QObject(parent) {}
  virtual ~IAppLogPresenter() = default;

  IAppLogPresenter(const IAppLogPresenter &) = delete;
  IAppLogPresenter &operator=(const IAppLogPresenter &) = delete;
  IAppLogPresenter(IAppLogPresenter &&) = delete;
  IAppLogPresenter &operator=(IAppLogPresenter &&) = delete;
};

Q_DECLARE_INTERFACE(IAppLogPresenter,
                    APPLOG_FEATURE_ID FEATURE_PRESENTER_SUFFIX)
