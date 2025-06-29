#include "CounterWidget.h"
#include "../../../events/LogEvent.h"
#include "../../../events/bus/EventBus.hpp"
#include <fmt/core.h>

CounterWidget::CounterWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::CounterWidget) {
  ui->setupUi(this);
  qDebug() << "CounterWidget instantiated";
}

CounterWidget::~CounterWidget() { delete ui; }

void CounterWidget::displayCounter(int value) {
  events::publish(
      LogEvent{fmt::format("CounterWidget::displayCounter({}) → "
                           "ui->counterLabel->setText(QString::number({}))",
                           value, value)});
  ui->counterLabel->setText(QString::number(value));
}

void CounterWidget::on_incrementButton_clicked() {
  events::publish(LogEvent{"CounterWidget::on_incrementButton_clicked() → "
                           "emit incrementRequested()"});
  emit incrementRequested();
}

void CounterWidget::on_resetButton_clicked() {
  events::publish(LogEvent{"CounterWidget::on_resetButton_clicked() → "
                           "emit resetRequested()"});
  emit resetRequested();
}

void CounterWidget::shutdown() { qInfo() << "CounterWidget::shutdown()"; }