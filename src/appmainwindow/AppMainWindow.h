#ifndef APP_MAIN_WINDOW_H
#define APP_MAIN_WINDOW_H

#include "ui_AppMainWindow.h"
#include <QMainWindow>

class ICounterModel;
class ICounterWidget;
class ICounterPresenter;

QT_BEGIN_NAMESPACE
namespace Ui {}
QT_END_NAMESPACE

class AppMainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit AppMainWindow(QWidget *parent = nullptr);
  ~AppMainWindow();

private:
  Ui::AppMainWindow *ui;

  ICounterModel *m_counterModel;
  ICounterWidget *m_counterWidgetView;
  ICounterPresenter *m_counterPresenter;
};

#endif