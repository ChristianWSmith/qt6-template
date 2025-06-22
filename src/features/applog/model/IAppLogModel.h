#pragma once

#include "../../IModel.h"
#include "../../featurescommon.h"
#include "../applogcommon.h"
#include <QMetaMethod>
#include <QObject>
#include <QString>
#include <QtPlugin>

class IAppLogModel : public QObject, public IModel {
  Q_OBJECT

public:
  explicit IAppLogModel(QObject *parent = nullptr) : QObject(parent) {}
  virtual ~IAppLogModel() = default;

  IAppLogModel(const IAppLogModel &) = delete;
  IAppLogModel &operator=(const IAppLogModel &) = delete;
  IAppLogModel(IAppLogModel &&) = delete;
  IAppLogModel &operator=(IAppLogModel &&) = delete;

  virtual void addLogMessage(const QString &message) = 0;

  virtual QMetaObject::Connection connectLogChanged(QObject *receiver,
                                                    const char *member) = 0;
};

Q_DECLARE_INTERFACE(IAppLogModel, APPLOG_FEATURE_ID FEATURE_MODEL_SUFFIX)
