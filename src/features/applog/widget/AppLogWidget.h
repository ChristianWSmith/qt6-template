#ifndef APPLOGWIDGET_H
#define APPLOGWIDGET_H
#include "IAppLogWidget.h"
#include "ui_AppLogWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui {}
QT_END_NAMESPACE

class AppLogWidget : public IAppLogWidget {
  Q_OBJECT
  Q_INTERFACES(IAppLogWidget)

public:
  explicit AppLogWidget(QWidget *parent = nullptr);
  ~AppLogWidget();
  // Implements IAppLogWidget methods

signals:
  // Signals emitted by this concrete Widget

private slots:
  // Slots for UI events (auto-connected by Qt Designer)

private:
  Ui::AppLogWidget *ui;
};

#endif
