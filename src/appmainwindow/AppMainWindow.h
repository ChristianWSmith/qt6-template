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
/// Ownership model:
///   - QObject feature objects (models, widgets, presenters): Qt parent-child
///     (parent = this). Widgets may be reparented into the layout container;
///     they remain owned by this window's QObject tree.
///   - Persistence provider: when the ctor's @p provider argument is nullptr,
///     a FilePersistenceProvider is constructed as a child of this window
///     (default path, owned via Qt parent-child). When a non-null provider is
///     injected, it is non-owning — the caller owns lifetime and it MUST
///     outlive this window. An injected provider is NOT reparented unless it
///     already has a suitable parent.
///   - Presenters hold non-owning QPointer refs to model and widget.
///   - Models hold non-owning IPersistenceProvider& to the provider.
///
/// Lifetime invariants:
///   - Member declaration order is CONSTRUCTION order (not a Qt destruction-order
///     guarantee). Qt does not guarantee arbitrary QObject destruction order.
///   - m_provider is bound before every model that references it.
///   - For each feature, model and widget are constructed before the presenter.
///   - Ownership is Qt parent-child: QObject children of this window die with it.
///   - Presenter pointers are non-owning. Presenter destructors must NOT
///     dereference m_model or m_view. Signal/slot connections auto-disconnect
///     when either QObject is destroyed.
///   - Any teardown logic that needs model/widget state must run before
///     destruction (e.g. closeEvent), not in presenter destructors.
class AppMainWindow : public QMainWindow {
  Q_OBJECT

public:
  /// @param provider Optional external persistence provider. When non-null the
  ///   caller owns it and it must outlive this window; AppMainWindow does not
  ///   reparent it. When nullptr (default), a FilePersistenceProvider is
  ///   constructed as a child of this window.
  /// @param parent Optional Qt parent widget.
  explicit AppMainWindow(IPersistenceProvider *provider = nullptr,
                         QWidget *parent = nullptr);
  ~AppMainWindow();

  AppMainWindow(const AppMainWindow &) = delete;
  AppMainWindow &operator=(const AppMainWindow &) = delete;
  AppMainWindow(AppMainWindow &&) = delete;
  AppMainWindow &operator=(AppMainWindow &&) = delete;

  void closeEvent(QCloseEvent *event) override;

private:
  std::unique_ptr<Ui::AppMainWindow> ui;

  // Bound before models (construction order). Non-owning: either the
  // caller-injected provider or the internally constructed child.
  IPersistenceProvider *m_provider;

  // Per feature: model and widget constructed before the presenter (non-owning refs).
  // Presenter destructors must not dereference these members.
  CounterModel *m_counterModel;
  CounterWidget *m_counterWidget;
  CounterPresenter *m_counterPresenter;

  AppLogModel *m_appLogModel;
  AppLogWidget *m_appLogWidget;
  AppLogPresenter *m_appLogPresenter;

  // Non-owning registry of feature models for polymorphic shutdown persistence
  // (F-09). Populated after feature construction in the ctor body. Qt parent-
  // child ownership remains with this window; these pointers do not own.
  QList<IModel *> m_models;
};
