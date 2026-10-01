#pragma once
#include "../countercommon.h"
#include "ui_CounterWidget.h"
#include <memory>
#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {}
QT_END_NAMESPACE

class CounterWidget : public QWidget {
  Q_OBJECT

public:
  explicit CounterWidget(QWidget *parent = nullptr);
  ~CounterWidget();

  CounterWidget(const CounterWidget &) = delete;
  CounterWidget &operator=(const CounterWidget &) = delete;
  CounterWidget(CounterWidget &&) = delete;
  CounterWidget &operator=(CounterWidget &&) = delete;

  void displayCounter(int value);

signals:
  void incrementRequested();
  void resetRequested();

private slots:
  void on_incrementButton_clicked();
  void on_resetButton_clicked();

private:
  std::unique_ptr<Ui::CounterWidget> ui;
};
