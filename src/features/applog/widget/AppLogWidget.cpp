#include "AppLogWidget.h"

AppLogWidget::AppLogWidget(QWidget *parent)
    : IAppLogWidget(parent), ui(new Ui::AppLogWidget) {
  ui->setupUi(this);
}

AppLogWidget::~AppLogWidget() { delete ui; }

// Implements IAppLogWidget methods and internal connections

// Implements UI slots, typically emitting signals to the Presenter
