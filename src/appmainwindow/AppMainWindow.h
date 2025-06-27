#pragma once

#include "../platform/FilePersistenceProvider.h"
#include "ui_AppMainWindow.h"
#include <QMainWindow>

class CounterModel;
class CounterWidget;
class CounterPresenter;

class AppLogModel;
class AppLogWidget;
class AppLogPresenter;

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

  FilePersistenceProvider *m_provider;

  CounterModel *m_counterModel;
  CounterWidget *m_counterWidget;
  CounterPresenter *m_counterPresenter;

  AppLogModel *m_appLogModel;
  AppLogWidget *m_appLogWidget;
  AppLogPresenter *m_appLogPresenter;
};
