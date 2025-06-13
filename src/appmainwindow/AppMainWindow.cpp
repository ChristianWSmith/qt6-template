
//---src/appmainwindow/AppMainWindow.cpp---

#include "AppMainWindow.h"

// Includes for Counter feature concrete classes
#include "../features/counter/model/CounterModel.h"
#include "../features/counter/presenter/CounterPresenter.h"
#include "../features/counter/widget/CounterWidget.h"

// Includes for AppLog feature concrete classes
#include "../features/applog/model/AppLogModel.h"
#include "../features/applog/presenter/AppLogPresenter.h"
#include "../features/applog/widget/AppLogWidget.h"

#include <QHBoxLayout> // Required for horizontal layout
#include <QWidget>     // Required for container widget

AppMainWindow::AppMainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::AppMainWindow) {
  ui->setupUi(this);

  setWindowTitle(APP_NAME);

  // Initialize Counter feature components
  m_counterModel = new CounterModel(this);
  m_counterWidgetView = new CounterWidget(this);
  m_counterPresenter =
      new CounterPresenter(m_counterModel, m_counterWidgetView, this);

  // Initialize AppLog feature components
  m_appLogModel = new AppLogModel(this);
  m_appLogWidgetView = new AppLogWidget(this);
  m_appLogPresenter =
      new AppLogPresenter(m_appLogModel, m_appLogWidgetView, this);

  // Create a container widget and set a horizontal layout
  // The central widget of a QMainWindow can only have one child,
  // so we use a container widget to hold multiple sub-widgets.
  QWidget *containerWidget = new QWidget(this);
  QHBoxLayout *mainLayout = new QHBoxLayout(containerWidget);

  // Add CounterWidget and AppLogWidget to the horizontal layout
  mainLayout->addWidget(m_counterWidgetView);
  mainLayout->addWidget(m_appLogWidgetView);

  // Set the container widget as the central widget of the main window
  setCentralWidget(containerWidget);
}

AppMainWindow::~AppMainWindow() { delete ui; }
