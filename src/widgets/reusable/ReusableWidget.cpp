#include "ReusableWidget.h"

ReusableWidget::ReusableWidget(QWidget *parent)
    : QWidget(parent), ui(std::make_unique<Ui::ReusableWidget>()) {
  ui->setupUi(this);
}

ReusableWidget::~ReusableWidget() = default;
