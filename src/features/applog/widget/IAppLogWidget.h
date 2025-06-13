#ifndef IAPPLOGWIDGET_H
#define IAPPLOGWIDGET_H
#include "../../featurescommon.h"
#include "../applogcommon.h"
#include <QWidget>
#include <QtPlugin>

class IAppLogWidget : public QWidget {
  Q_OBJECT

public:
  explicit IAppLogWidget(QWidget *parent = nullptr) : QWidget(parent) {}
  virtual ~IAppLogWidget() = default;
  // Widget API methods and connections
};

Q_DECLARE_INTERFACE(IAppLogWidget, APPLOG_FEATURE_ID FEATURE_WIDGET_SUFFIX)

#endif
