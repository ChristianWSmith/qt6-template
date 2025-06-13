#include "AppMainWindow.h"

#include "../features/counter/model/CounterModel.h"
#include "../features/counter/presenter/CounterPresenter.h"
#include "../features/counter/widget/CounterWidget.h"

#include "../features/applog/model/AppLogModel.h"
#include "../features/applog/presenter/AppLogPresenter.h"
#include "../features/applog/widget/AppLogWidget.h"

#include <QHBoxLayout>
#include <QWidget>

AppMainWindow::AppMainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::AppMainWindow) {
  ui->setupUi(this);

  setWindowTitle(APP_NAME);

  m_counterModel = new CounterModel(this);
  m_counterWidgetView = new CounterWidget(this);
  m_counterPresenter =
      new CounterPresenter(m_counterModel, m_counterWidgetView, this);

  m_appLogModel = new AppLogModel(this);
  m_appLogWidgetView = new AppLogWidget(this);
  m_appLogPresenter =
      new AppLogPresenter(m_appLogModel, m_appLogWidgetView, this);

  QWidget *containerWidget = new QWidget(this);
  QHBoxLayout *mainLayout = new QHBoxLayout(containerWidget);

  mainLayout->addWidget(m_counterWidgetView);
  mainLayout->addWidget(m_appLogWidgetView);

  setCentralWidget(containerWidget);
}

AppMainWindow::~AppMainWindow() { delete ui; }
