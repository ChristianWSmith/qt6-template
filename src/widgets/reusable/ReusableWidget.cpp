#include "ReusableWidget.h"

ReusableWidget::ReusableWidget(QWidget *parent)
    : IReusableWidget(parent), ui(new Ui::ReusableWidget) {
  ui->setupUi(this);
}

ReusableWidget::~ReusableWidget() { delete ui; }

// Implements IReusableWidget methods and internal connections

// Implements UI slots, typically emitting signals to the Presenter

void ReusableWidget::shutdown() {}
