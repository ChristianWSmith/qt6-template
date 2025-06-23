#pragma once

#include "../../IWidget.h"
#include "../../featurescommon.h"
#include "../applogcommon.h"
#include <QString>
#include <QWidget>
#include <QtPlugin>

class IAppLogWidget : public QWidget, public IWidget {
  Q_OBJECT

public:
  explicit IAppLogWidget(QWidget *parent = nullptr) : QWidget(parent) {}
  virtual ~IAppLogWidget() = default;

  IAppLogWidget(const IAppLogWidget &) = delete;
  IAppLogWidget &operator=(const IAppLogWidget &) = delete;
  IAppLogWidget(IAppLogWidget &&) = delete;
  IAppLogWidget &operator=(IAppLogWidget &&) = delete;

  virtual QMetaObject::Connection connectClearRequested(QObject *receiver,
                                                        const char *member) = 0;
  virtual void clear() = 0;
  virtual void setLogMessages(const QVector<QString> &messages) = 0;

public slots:
  virtual void handleLogChanged(const LogDelta &logDelta) = 0;
};

Q_DECLARE_INTERFACE(IAppLogWidget, APPLOG_FEATURE_ID FEATURE_WIDGET_SUFFIX)
