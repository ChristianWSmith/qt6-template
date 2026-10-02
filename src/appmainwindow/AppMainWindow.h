#pragma once

#include "../core/IPersistenceProvider.h"
#include "../core/IModel.h"
#include "../platform/core/FilePersistenceProvider.h"
#include "ui_AppMainWindow.h"
#include <QList>
#include <QMainWindow>
#include <memory>

#include "../features/counter/model/CounterModel.h"
#include "../features/counter/presenter/CounterPresenter.h"
#include "../features/counter/widget/CounterWidget.h"

#include "../features/applog/model/AppLogModel.h"
#include "../features/applog/presenter/AppLogPresenter.h"
#include "../features/applog/widget/AppLogWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui {}
QT_END_NAMESPACE

/// Composition root. Owns feature objects via Qt parent-child ownership.
///
/// Ownership model (two ctor overloads encode the persistence-seam mode):
///   - QObject feature objects (models, widgets, presenters): Qt parent-child
///     (parent = this). Widgets may be reparented into the layout container;
///     they remain owned by this window's QObject tree.
///   - Persistence provider: two explicit ownership modes are encoded in the
///     constructor overloads (no dual-mode nullptr pointer):
///     - Default ctor: a FilePersistenceProvider is constructed as a Qt child
///       of this window (owned via Qt parent-child).
///     - Injected ctor (IPersistenceProvider&): caller-owned; it MUST outlive
///       this window and is NOT reparented. Declare the provider BEFORE the
///       window in caller scope. Models bind IPersistenceProvider& to this
///       reference.
///   - Presenters hold non-owning QPointer refs to model and widget.
///   - Models hold non-owning IPersistenceProvider& to the provider.
///
/// Lifetime invariants:
///   - Member declaration order is CONSTRUCTION order (not a Qt destruction-order
///     guarantee). Qt does not guarantee arbitrary QObject destruction order.
///   - m_provider is bound before every model that references it.
///   - For each feature, model and widget are constructed before the presenter
///     (single wiring site: bindFeatures()).
///   - Ownership is Qt parent-child: QObject children of this window die with it.
///   - Presenter pointers are non-owning. Presenter destructors must NOT
///     dereference m_model or m_view. Signal/slot connections auto-disconnect
///     when either QObject is destroyed.
///   - Any teardown logic that needs model/widget state must run before
///     destruction (e.g. closeEvent), not in presenter destructors.
///   - Model destructors and teardown-reachable code must NOT call the
///     persistence provider. Persistence I/O is only valid during loadState
///     (ctor) and saveState() while the provider is guaranteed alive.
///   - Every entry in m_models must outlive AppMainWindow. Do not destroy
///     feature models independently while the window is alive.
///
/// Lifetime contracts (Wave 1):
///   - AppMainWindow is stack-allocated in main(). Do NOT set
///     Qt::WA_DeleteOnClose — the stack frame would double-destroy the window.
///   - closeEvent intentionally calls hide() first (visible-state contract;
///     covered by lifecycle tests).
///   - Feature members are nullptr until bindFeatures() wires them.
class AppMainWindow : public QMainWindow {
  Q_OBJECT

public:
  /// Default: constructs FilePersistenceProvider as a Qt child of this window.
  explicit AppMainWindow(QWidget *parent = nullptr);

  /// Injected: caller-owned provider; must outlive this window; NOT reparented.
  /// Declare the provider before the window in caller scope.
  /// Models bind IPersistenceProvider& to this reference.
  explicit AppMainWindow(IPersistenceProvider &provider, QWidget *parent = nullptr);
  ~AppMainWindow();

  AppMainWindow(const AppMainWindow &) = delete;
  AppMainWindow &operator=(const AppMainWindow &) = delete;
  AppMainWindow(AppMainWindow &&) = delete;
  AppMainWindow &operator=(AppMainWindow &&) = delete;

  // Hides first (visible-state contract) then saves chrome + feature state
  // via the m_models loop; never blocks close on persistence failure.
  void closeEvent(QCloseEvent *event) override;

private:
  // Single wiring site for both ctor overloads: construct feature objects
  // (model → widget → presenter), append models to m_models, add widgets to
  // the central layout. Call after ui setup is NOT required for construction;
  // layout add uses already-constructed widgets.
  void bindFeatures(IPersistenceProvider &provider);

  // Shared post-wiring for both ctor overloads: ui setup, m_models registry
  // (already populated by bindFeatures), NDEBUG assert, QSettings chrome
  // restore, window title, layout container.
  void finishConstruction();

  std::unique_ptr<Ui::AppMainWindow> ui;

  // Bound before models (bindFeatures). Non-owning: either the
  // caller-injected provider (borrowed, must outlive this window) or the
  // internally constructed child (owned by Qt parent-child).
  IPersistenceProvider *m_provider = nullptr;

  // Per feature: model and widget constructed before the presenter (non-owning refs).
  // Presenter destructors must not dereference these pointers.
  // nullptr until bindFeatures() runs.
  CounterModel *m_counterModel = nullptr;
  CounterWidget *m_counterWidget = nullptr;
  CounterPresenter *m_counterPresenter = nullptr;

  AppLogModel *m_appLogModel = nullptr;
  AppLogWidget *m_appLogWidget = nullptr;
  AppLogPresenter *m_appLogPresenter = nullptr;

  // Non-owning registry of feature models for polymorphic shutdown persistence.
  // Populated in bindFeatures(). Qt parent-child ownership remains with this
  // window; these pointers do not own. Entries must outlive the window.
  QList<IModel *> m_models;
};
