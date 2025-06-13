//---src/appmainwindow/AppMainWindow.h---
#ifndef APP_MAIN_WINDOW_H
#define APP_MAIN_WINDOW_H

#include "ui_AppMainWindow.h"
#include <QMainWindow>

// Forward declarations for Counter feature
class ICounterModel;
class ICounterWidget;
class ICounterPresenter;

// Forward declarations for AppLog feature
class IAppLogModel;
class IAppLogWidget;
class IAppLogPresenter;

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

  // Counter feature components
  ICounterModel *m_counterModel;
  ICounterWidget *m_counterWidgetView;
  ICounterPresenter *m_counterPresenter;

  // AppLog feature components
  IAppLogModel *m_appLogModel;
  IAppLogWidget *m_appLogWidgetView;
  IAppLogPresenter *m_appLogPresenter;
};

#endif