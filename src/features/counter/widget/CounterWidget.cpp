#include "CounterWidget.h"
#include <fmt/core.h>

CounterWidget::CounterWidget(QWidget *parent)
    : ICounterWidget(parent), ui(new Ui::CounterWidget) {
  ui->setupUi(this);
}

CounterWidget::~CounterWidget() { delete ui; }

void CounterWidget::displayCounter(int value) {
  ui->counterLabel->setText(QString::number(value));
}

void CounterWidget::on_incrementButton_clicked() { emit incrementRequested(); }

QMetaObject::Connection
CounterWidget::connectIncrementRequested(QObject *receiver,
                                         const char *member) {
  return QObject::connect(this, SIGNAL(incrementRequested()), receiver, member);
}
