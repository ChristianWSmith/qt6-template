#include "AppMainWindow.h"

#include <QCloseEvent>
#include <QMainWindow>
#include <QSettings>

#include <QHBoxLayout>
#include <QWidget>

AppMainWindow::AppMainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::AppMainWindow),
      m_provider(new FilePersistenceProvider(this)),
      m_counterModel(new CounterModel(m_provider, this)),
      m_counterWidget(new CounterWidget(this)),
      m_counterPresenter(
          new CounterPresenter(m_counterModel, m_counterWidget, this)),
      m_appLogModel(new AppLogModel(m_provider, this)),
      m_appLogWidget(new AppLogWidget(this)),
      m_appLogPresenter(
          new AppLogPresenter(m_appLogModel, m_appLogWidget, this)) {

  ui->setupUi(this);

  QSettings settings(ORGANIZATION_NAME, APP_NAME);
  restoreGeometry(settings.value("window/geometry").toByteArray());
  restoreState(settings.value("window/state").toByteArray());

  setWindowTitle(APP_NAME);

  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
  auto *containerWidget = new QWidget(this);

  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
  auto *mainLayout = new QHBoxLayout(containerWidget);

  mainLayout->addWidget(m_counterWidget);

  mainLayout->addWidget(m_appLogWidget);

  setCentralWidget(containerWidget);
}

AppMainWindow::~AppMainWindow() { delete ui; }

void AppMainWindow::closeEvent(QCloseEvent *event) {
  this->hide();
  qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

  QSettings settings(ORGANIZATION_NAME, APP_NAME);
  settings.setValue("window/geometry", saveGeometry());
  settings.setValue("window/state", saveState());

  m_counterModel->saveState();
  m_appLogModel->saveState();

  QMainWindow::closeEvent(event);
}
