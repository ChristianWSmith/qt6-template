#include "AppMainWindow.h"

#include <QCloseEvent>
#include <QMainWindow>
#include <QSettings>

#include <QFuture>
#include <QHBoxLayout>
#include <QWidget>
#include <QtConcurrent/QtConcurrent>

AppMainWindow::AppMainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::AppMainWindow),
      m_provider(new FilePersistenceProvider()),
      m_appLogModel(new AppLogModel(m_provider, this)),
      m_appLogPresenter(
          new AppLogPresenter(m_appLogModel, m_appLogWidget, this)),
      m_appLogWidget(new AppLogWidget(this)),
      m_counterModel(new CounterModel(m_provider, this)),
      m_counterPresenter(
          new CounterPresenter(m_counterModel, m_counterWidget, this)),
      m_counterWidget(new CounterWidget(this)) {

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

void AppMainWindow::shutdown() {
  QSettings settings(ORGANIZATION_NAME, APP_NAME);
  settings.setValue("window/geometry", saveGeometry());
  settings.setValue("window/state", saveState());
}

void AppMainWindow::closeEvent(QCloseEvent *event) {
  this->hide();
  qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

  QFuture<void> shutdownFuture = QtConcurrent::run([this]() { shutdown(); });
  QFuture<void> counterFuture =
      QtConcurrent::run([this]() { m_counterPresenter->shutdown(); });
  QFuture<void> appLogFuture =
      QtConcurrent::run([this]() { m_appLogPresenter->shutdown(); });

  shutdownFuture.waitForFinished();
  counterFuture.waitForFinished();
  appLogFuture.waitForFinished();

  QMainWindow::closeEvent(event);
}
