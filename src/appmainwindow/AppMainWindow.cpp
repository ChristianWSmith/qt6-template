#include "AppMainWindow.h"

#include "logging/logging.h"

#include <QCloseEvent>
#include <QMainWindow>
#include <QSettings>

#include <QHBoxLayout>
#include <QWidget>

AppMainWindow::AppMainWindow(IPersistenceProvider *provider, QWidget *parent)
    : QMainWindow(parent), ui(std::make_unique<Ui::AppMainWindow>()),
      // Provider first: models bind a non-owning IPersistenceProvider&.
      // External providers are non-owning (caller-owned, must outlive this
      // window); nullptr constructs a FilePersistenceProvider child of this
      // window (default path). Do not reparent an injected provider.
      m_provider(provider != nullptr ? provider
                                    : new FilePersistenceProvider(this)),
      m_counterModel(new CounterModel(*m_provider, this)),
      m_counterWidget(new CounterWidget(this)),
      m_counterPresenter(
          new CounterPresenter(m_counterModel, m_counterWidget, this)),
      m_appLogModel(new AppLogModel(*m_provider, this)),
      m_appLogWidget(new AppLogWidget(this)),
      m_appLogPresenter(
          new AppLogPresenter(m_appLogModel, m_appLogWidget, this)) {

  ui->setupUi(this);

  // F-09: polymorphic IModel registry for shutdown persistence.
  // Individual pointers remain for wiring/feature access; this list drives
  // closeEvent's save loop over every feature model.
  m_models << m_counterModel << m_appLogModel;

  QSettings settings(ORGANIZATION_NAME, APP_NAME);
  restoreGeometry(settings.value("window/geometry").toByteArray());
  restoreState(settings.value("window/state").toByteArray());

  setWindowTitle(APP_NAME);

  // Central layout is code-built (feature widgets are constructed in code).
  // AppMainWindow.ui is intentionally minimal — geometry/windowTitle only.
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
  auto *containerWidget = new QWidget(this);

  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
  auto *mainLayout = new QHBoxLayout(containerWidget);

  mainLayout->addWidget(m_counterWidget);

  mainLayout->addWidget(m_appLogWidget);

  setCentralWidget(containerWidget);
}

AppMainWindow::~AppMainWindow() = default;

void AppMainWindow::closeEvent(QCloseEvent *event) {
  this->hide();

  QSettings settings(ORGANIZATION_NAME, APP_NAME);
  settings.setValue("window/geometry", saveGeometry());
  settings.setValue("window/state", saveState());

  // F-01/F-09: observe save results polymorphically.
  // Provider logging ownership stays with FilePersistenceProvider — do not
  // duplicate qCWarning here for the same failure. Default shutdown policy
  // remains log-and-continue: do not block close on persistence failure.
  for (IModel *model : m_models) {
    const auto result = model->saveState();
    if (result.hasError()) {
      qCDebug(appPersistence)
          << "closeEvent: feature save reported failure (provider already logged):"
          << toString(result.error());
    }
  }

  QMainWindow::closeEvent(event);
}
