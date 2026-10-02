#include "AppMainWindow.h"

#include "logging/logging.h"

#include <QCloseEvent>
#include <QMainWindow>
#include <QSettings>

#include <QHBoxLayout>
#include <QWidget>

AppMainWindow::AppMainWindow(QWidget *parent)
    : QMainWindow(parent), ui(std::make_unique<Ui::AppMainWindow>()),
      // Provider first: models bind a non-owning IPersistenceProvider&.
      // Default path: owned FilePersistenceProvider child of this window
      // (sibling Qt children; convention-only outliving — models must not
      // touch the provider in their destructors).
      // Constructed before every model that references it.
      m_provider(new FilePersistenceProvider(this)) {
  constructFeatures();
  finishConstruction();
}

AppMainWindow::AppMainWindow(IPersistenceProvider &provider, QWidget *parent)
    : QMainWindow(parent), ui(std::make_unique<Ui::AppMainWindow>()),
      // Injected path: borrow the caller-owned provider. Do NOT reparent and
      // do NOT take ownership — the caller must outlive this window
      // (structural: caller stack order declares provider before window).
      m_provider(&provider) {
  constructFeatures();
  finishConstruction();
}

void AppMainWindow::registerModel(IModel *model) {
  Q_ASSERT(model != nullptr);
  m_models << model;
}

void AppMainWindow::constructFeatures() {
  // Sole composition-wiring site. Provider is already bound (m_provider set
  // in the init list). Per feature: model → widget → presenter (presenter
  // does not create model/widget). Models load persisted state in their
  // constructors before the presenter exists.
  m_counterModel = new CounterModel(*m_provider, this);
  m_counterWidget = new CounterWidget(this);
  m_counterPresenter =
      new CounterPresenter(m_counterModel, m_counterWidget, this);
  registerModel(m_counterModel);

  m_appLogModel = new AppLogModel(*m_provider, this);
  m_appLogWidget = new AppLogWidget(this);
  m_appLogPresenter = new AppLogPresenter(m_appLogModel, m_appLogWidget, this);
  registerModel(m_appLogModel);
}

void AppMainWindow::finishConstruction() {
  ui->setupUi(this);

  QSettings settings(ORGANIZATION_NAME, APP_NAME);
  restoreGeometry(settings.value("window/geometry").toByteArray());
  restoreState(settings.value("window/state").toByteArray());
  if (settings.status() != QSettings::NoError) {
    qCWarning(appMain) << "QSettings chrome operation failed:"
                       << settings.status();
  }

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
  if (settings.status() != QSettings::NoError) {
    qCWarning(appMain) << "QSettings chrome operation failed:"
                       << settings.status();
  }

  // Observe save results polymorphically.
  // Provider logging ownership stays with FilePersistenceProvider — do not
  // duplicate qCWarning here for the same failure. Default shutdown policy
  // remains log-and-continue: do not block close on persistence failure.
  for (IModel *model : m_models) {
    const auto result = model->saveState();
    if (result.hasError()) {
      qCDebug(appPersistence) << "closeEvent: feature save reported failure "
                                 "(provider already logged):"
                              << toString(result.error());
    }
  }

  QMainWindow::closeEvent(event);
}
