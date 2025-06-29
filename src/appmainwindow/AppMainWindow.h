#pragma once

#include "../platform/FilePersistenceProvider.h"
#include "ui_AppMainWindow.h"
#include <QMainWindow>

#include "../features/counter/model/CounterModel.h"
#include "../features/counter/presenter/CounterPresenter.h"
#include "../features/counter/widget/CounterWidget.h"

#include "../features/applog/model/AppLogModel.h"
#include "../features/applog/presenter/AppLogPresenter.h"
#include "../features/applog/widget/AppLogWidget.h"

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
