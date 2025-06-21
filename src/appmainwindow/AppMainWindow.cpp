#include "AppMainWindow.h"

#include "../features/counter/model/CounterModel.h"
#include "../features/counter/presenter/CounterPresenter.h"
#include "../features/counter/widget/CounterWidget.h"

#include "../features/applog/model/AppLogModel.h"
#include "../features/applog/presenter/AppLogPresenter.h"
#include "../features/applog/widget/AppLogWidget.h"
#include <QCloseEvent>
#include <QMainWindow>
#include <QSettings>

#include <QHBoxLayout>
#include <QWidget>
#include <qsettings.h>

AppMainWindow::AppMainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::AppMainWindow) {

  ui->setupUi(this);

  QSettings settings(APP_ID, APP_NAME);
  restoreGeometry(settings.value("window/geometry").toByteArray());
  restoreState(settings.value("window/state").toByteArray());

  setWindowTitle(APP_NAME);

  m_counterModel = new CounterModel(this);
  m_counterWidget = new CounterWidget(this);
  m_counterPresenter =
      new CounterPresenter(m_counterModel, m_counterWidget, this);

  m_appLogModel = new AppLogModel(this);
  m_appLogWidget = new AppLogWidget(this);
  m_appLogPresenter = new AppLogPresenter(m_appLogModel, m_appLogWidget, this);

  QWidget *containerWidget = new QWidget(this);

  QHBoxLayout *mainLayout = new QHBoxLayout(containerWidget);

  mainLayout->addWidget(m_counterWidget);

  mainLayout->addWidget(m_appLogWidget);

  setCentralWidget(containerWidget);
}

AppMainWindow::~AppMainWindow() { delete ui; }

void AppMainWindow::closeEvent(QCloseEvent *event) {
  QSettings settings(APP_ID, APP_NAME);
  settings.setValue("window/geometry", saveGeometry());
  settings.setValue("window/state", saveState());

  m_counterPresenter->shutdown();
  m_appLogPresenter->shutdown();

  QMainWindow::closeEvent(event);
}
