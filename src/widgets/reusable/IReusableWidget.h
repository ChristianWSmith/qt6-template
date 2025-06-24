#pragma once
#include "../../core/IWidget.h"
#include "../../core/interfacecommon.h"
#include "reusablecommon.h"
#include <QWidget>
#include <QtPlugin>

class IReusableWidget : public QWidget, public IWidget {
  Q_OBJECT

public:
  explicit IReusableWidget(QWidget *parent = nullptr) : QWidget(parent) {}
  virtual ~IReusableWidget() = default;

  IReusableWidget(const IReusableWidget &) = delete;
  IReusableWidget &operator=(const IReusableWidget &) = delete;
  IReusableWidget(IReusableWidget &&) = delete;
  IReusableWidget &operator=(IReusableWidget &&) = delete;
  // Widget API methods and connections
};

Q_DECLARE_INTERFACE(IReusableWidget, REUSABLE_WIDGET_ID)
