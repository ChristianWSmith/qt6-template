#pragma once

#include "../platform/core/FilePersistenceProvider.h"
#include "ui_AppMainWindow.h"
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
///   - Member declaration order is construction order.
///   - m_provider is constructed before every model that references it.
///   - For each feature, model and widget are constructed before the
///     presenter; the presenter does not outlive its dependencies.
///   - Destruction of QObject children is handled by Qt parent-child after
///     this destructor body; model/presenter destructors must not rely on
///     outliving each other beyond the construction order above.
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

  // Construction order encodes lifetime dependency: provider before models.
  FilePersistenceProvider *m_provider;

  // Per feature: model and widget before presenter (non-owning presenter refs).
  CounterModel *m_counterModel;
  CounterWidget *m_counterWidget;
  CounterPresenter *m_counterPresenter;

  AppLogModel *m_appLogModel;
  AppLogWidget *m_appLogWidget;
  AppLogPresenter *m_appLogPresenter;
};
