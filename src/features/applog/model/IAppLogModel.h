#ifndef IAPPLOGMODEL_H
#define IAPPLOGMODEL_H
#include "../../featurescommon.h"
#include "../applogcommon.h"
#include <QMetaMethod>
#include <QObject>
#include <QtPlugin>

class IAppLogModel : public QObject {
  Q_OBJECT

public:
  explicit IAppLogModel(QObject *parent = nullptr) : QObject(parent) {}
  virtual ~IAppLogModel() = default;
  // Model API methods and connections
};

Q_DECLARE_INTERFACE(IAppLogModel, APPLOG_FEATURE_ID FEATURE_MODEL_SUFFIX)

#endif
