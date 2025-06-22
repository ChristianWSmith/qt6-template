#pragma once
#include "ICounterWidget.h"
#include "ui_CounterWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui {}
QT_END_NAMESPACE

class CounterWidget : public ICounterWidget {
  Q_OBJECT
  Q_INTERFACES(ICounterWidget)

public:
  explicit CounterWidget(QWidget *parent = nullptr);
  ~CounterWidget();

  CounterWidget(const CounterWidget &) = delete;
  CounterWidget &operator=(const CounterWidget &) = delete;
  CounterWidget(CounterWidget &&) = delete;
  CounterWidget &operator=(CounterWidget &&) = delete;
  void shutdown() override;

  void displayCounter(int value) override;
  QMetaObject::Connection
  connectIncrementRequested(QObject *receiver, const char *member) override;

signals:
  void incrementRequested();

private slots:
  void on_incrementButton_clicked();

private:
  Ui::CounterWidget *ui;
};
