#pragma once
#include "../countercommon.h"
#include "../model/CounterModel.h"
#include "../widget/CounterWidget.h"
#include <QObject>
#include <QPointer>

class CounterPresenter : public QObject {
  Q_OBJECT

public:
  // Ctor-by-reference encodes required non-null dependencies (matches models).
  // QPointer members still observe mid-session destruction.
  explicit CounterPresenter(CounterModel &model, CounterWidget &view,
                            QObject *parent = nullptr);

private slots:
  void handleIncrementRequest();
  void handleResetRequest();
  void handleCounterValueChanged(int newValue);

private:
  // Non-owning QPointer refs. Owned via Qt parent-child under AppMainWindow.
  // Slots null-guard as belt-and-suspenders: Qt auto-disconnects QObject
  // connections on destruction, but the EventSystem path and partial-
  // destruction tests make the guard load-bearing there. Presenter
  // destructors must not dereference these pointers.
  QPointer<CounterModel> m_model;
  QPointer<CounterWidget> m_view;
};
