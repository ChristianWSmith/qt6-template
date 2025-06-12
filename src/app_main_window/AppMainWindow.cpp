
#include "AppMainWindow.h"

#include "../features/counter/model/CounterModel.h"
#include "../features/counter/presenter/CounterPresenter.h"
#include "../features/counter/widget/CounterWidget.h"

AppMainWindow::AppMainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::AppMainWindow) {
  ui->setupUi(this);

  setWindowTitle(APP_NAME);

  m_counterModel = new CounterModel(this);
  m_counterWidgetView = new CounterWidget(this);

  m_counterPresenter =
      new CounterPresenter(m_counterModel, m_counterWidgetView, this);

  setCentralWidget(m_counterWidgetView);
}

AppMainWindow::~AppMainWindow() { delete ui; }
