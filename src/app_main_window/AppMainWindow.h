#ifndef APP_MAIN_WINDOW_H
#define APP_MAIN_WINDOW_H

#include "ui_AppMainWindow.h"
#include <QMainWindow>

class Counter;
class CounterWidget;
class CounterPresenter;

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

  Counter *m_counterModel;
  CounterWidget *m_counterWidgetView;
  CounterPresenter *m_counterPresenter;
};

#endif