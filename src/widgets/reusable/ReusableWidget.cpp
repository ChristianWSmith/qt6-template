#include "ReusableWidget.h"

ReusableWidget::ReusableWidget(QWidget *parent) : ui(new Ui::ReusableWidget) {
  ui->setupUi(this);
}

ReusableWidget::~ReusableWidget() { delete ui; }

// Implements UI slots, typically emitting signals to the Presenter

void ReusableWidget::shutdown() {}
