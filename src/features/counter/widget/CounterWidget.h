#pragma once
#include "../../../core/IWidget.h"
#include "../countercommon.h"
#include "ui_CounterWidget.h"
#include <QWidget>
#include <QtPlugin>

QT_BEGIN_NAMESPACE
namespace Ui {}
QT_END_NAMESPACE

class CounterWidget : public QWidget, public IWidget {
  Q_OBJECT

public:
  explicit CounterWidget(QWidget *parent = nullptr);
  ~CounterWidget();

  CounterWidget(const CounterWidget &) = delete;
  CounterWidget &operator=(const CounterWidget &) = delete;
  CounterWidget(CounterWidget &&) = delete;
  CounterWidget &operator=(CounterWidget &&) = delete;

  void shutdown() override;

  void displayCounter(int value);

signals:
  void incrementRequested();
  void resetRequested();

private slots:
  void on_incrementButton_clicked();
  void on_resetButton_clicked();

private:
  friend class CounterTest;
  Ui::CounterWidget *ui;
};
