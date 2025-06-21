#pragma once

#include "ui_AppMainWindow.h"
#include <QMainWindow>

class ICounterModel;
class ICounterWidget;
class ICounterPresenter;

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
  void closeEvent(QCloseEvent *event);

private:
  Ui::AppMainWindow *ui;

  ICounterModel *m_counterModel;
  ICounterWidget *m_counterWidget;
  ICounterPresenter *m_counterPresenter;

  IAppLogModel *m_appLogModel;
  IAppLogWidget *m_appLogWidget;
  IAppLogPresenter *m_appLogPresenter;
};
