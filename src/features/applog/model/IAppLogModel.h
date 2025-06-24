#pragma once

#include "../../../core/IModel.h"
#include "../../../core/interfacecommon.h"
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
  virtual void clear() = 0;
  [[nodiscard]] virtual const QVector<QString> &getLogMessages() const = 0;

  virtual QMetaObject::Connection connectLogChanged(QObject *receiver,
                                                    const char *member) = 0;
  virtual QMetaObject::Connection connectLogCleared(QObject *receiver,
                                                    const char *member) = 0;
};

Q_DECLARE_INTERFACE(IAppLogModel, APPLOG_FEATURE_ID QINTERFACE_MODEL_SUFFIX)
