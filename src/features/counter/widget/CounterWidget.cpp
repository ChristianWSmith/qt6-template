#include "CounterWidget.h"
#include "../../../logging/logging.h"

CounterWidget::CounterWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::CounterWidget) {
  ui->setupUi(this);
  qCDebug(appFeature) << "CounterWidget instantiated";
}

CounterWidget::~CounterWidget() { delete ui; }

void CounterWidget::displayCounter(int value) {
  ui->counterLabel->setText(QString::number(value));
}

void CounterWidget::on_incrementButton_clicked() { emit incrementRequested(); }

void CounterWidget::on_resetButton_clicked() { emit resetRequested(); }
