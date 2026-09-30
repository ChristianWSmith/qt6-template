#pragma once

#include "../core/IModel.h"
#include "../platform/core/FilePersistenceProvider.h"
#include "ui_AppMainWindow.h"
#include <QList>
#include <QMainWindow>

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
///   - QObject feature objects (provider, models, widgets, presenters):
///     Qt parent-child (parent = this). Widgets may be reparented into the
///     layout container; they remain owned by this window's QObject tree.
///   - Presenters hold non-owning raw pointers to model and widget.
///   - Models hold non-owning IPersistenceProvider& to the provider.
///
/// Lifetime invariants:
///   - Member declaration order is CONSTRUCTION order (not a Qt destruction-order
///     guarantee). Qt does not guarantee arbitrary QObject destruction order.
///   - m_provider is constructed before every model that references it.
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
  explicit AppMainWindow(QWidget *parent = nullptr);
  ~AppMainWindow();

  AppMainWindow(const AppMainWindow &) = delete;
  AppMainWindow &operator=(const AppMainWindow &) = delete;
  AppMainWindow(AppMainWindow &&) = delete;
  AppMainWindow &operator=(AppMainWindow &&) = delete;

  void closeEvent(QCloseEvent *event) override;

private:
  Ui::AppMainWindow *ui;

  // Construction order: provider before models. Not a destruction-order guarantee.
  FilePersistenceProvider *m_provider;

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
