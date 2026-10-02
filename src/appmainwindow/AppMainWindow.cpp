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
      // Default path: owned FilePersistenceProvider child of this window.
      // Constructed before every model that references it.
      m_provider(new FilePersistenceProvider(this)) {
  bindFeatures(*m_provider);
  finishConstruction();
}

AppMainWindow::AppMainWindow(IPersistenceProvider &provider, QWidget *parent)
    : QMainWindow(parent), ui(std::make_unique<Ui::AppMainWindow>()),
      // Injected path: borrow the caller-owned provider. Do NOT reparent and
      // do NOT take ownership — the caller must outlive this window.
      // Declare the provider before the window in caller scope.
      m_provider(&provider) {
  bindFeatures(*m_provider);
  finishConstruction();
}

void AppMainWindow::bindFeatures(IPersistenceProvider &provider) {
  // Single wiring site for both ctor overloads (D-002).
  // Per feature: model → widget → presenter; then m_models + layout.
  m_counterModel = new CounterModel(provider, this);
  m_counterWidget = new CounterWidget(this);
  m_counterPresenter =
      new CounterPresenter(*m_counterModel, *m_counterWidget, this);

  m_appLogModel = new AppLogModel(provider, this);
  m_appLogWidget = new AppLogWidget(this);
  m_appLogPresenter =
      new AppLogPresenter(*m_appLogModel, *m_appLogWidget, this);

  // Polymorphic IModel registry for shutdown persistence.
  m_models << m_counterModel << m_appLogModel;
}

void AppMainWindow::finishConstruction() {
  ui->setupUi(this);

  // Debug: catch forgotten m_models appends (silent persistence opt-in).
  // IModel is a pure interface (not QObject); feature models inherit both
  // QObject and IModel, so a generic sweep via dynamic_cast works for ANY
  // feature type — not only the types known at composition-root compile time.
#ifndef NDEBUG
  int modelChildren = 0;
  const QList<QObject *> children = findChildren<QObject *>();
  for (QObject *child : children) {
    if (dynamic_cast<IModel *>(child) != nullptr) {
      ++modelChildren;
    }
  }
  Q_ASSERT(modelChildren == m_models.size());
#endif

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

  // Observe save results polymorphically.
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
