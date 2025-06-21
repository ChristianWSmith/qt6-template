#pragma once

#include "../../featurescommon.h"
#include "../applogcommon.h"
#include <QString>
#include <QWidget>
#include <QtPlugin>

class IAppLogWidget : public QWidget {
  Q_OBJECT

public:
  explicit IAppLogWidget(QWidget *parent = nullptr) : QWidget(parent) {}
  virtual ~IAppLogWidget() = default;

public slots:
  virtual void displayLogMessage(const QString &message) = 0;
};

Q_DECLARE_INTERFACE(IAppLogWidget, APPLOG_FEATURE_ID FEATURE_WIDGET_SUFFIX)
