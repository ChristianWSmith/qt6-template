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

  AppMainWindow(const AppMainWindow &) = delete;
  AppMainWindow &operator=(const AppMainWindow &) = delete;
  AppMainWindow(AppMainWindow &&) = delete;
  AppMainWindow &operator=(AppMainWindow &&) = delete;

  void closeEvent(QCloseEvent *event);
  void shutdown();

private:
  Ui::AppMainWindow *ui;

  ICounterModel *m_counterModel;
  ICounterWidget *m_counterWidget;
  ICounterPresenter *m_counterPresenter;

  IAppLogModel *m_appLogModel;
  IAppLogWidget *m_appLogWidget;
  IAppLogPresenter *m_appLogPresenter;
};
