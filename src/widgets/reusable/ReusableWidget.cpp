#include "ReusableWidget.h"

ReusableWidget::ReusableWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::ReusableWidget) {
  ui->setupUi(this);
}

ReusableWidget::~ReusableWidget() { delete ui; }
