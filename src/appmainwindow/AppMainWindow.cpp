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
