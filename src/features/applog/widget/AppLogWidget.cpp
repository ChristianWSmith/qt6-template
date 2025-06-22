#include "AppLogWidget.h"
#include <QScrollBar>
#include <fmt/core.h>

AppLogWidget::AppLogWidget(QWidget *parent)
    : IAppLogWidget(parent), ui(new Ui::AppLogWidget) {
  ui->setupUi(this);
  qDebug() << "AppLogWidget instantiated";
}

AppLogWidget::~AppLogWidget() { delete ui; }

void AppLogWidget::handleLogChanged(const LogChange &logChange) {
  QScrollBar *scrollBar = ui->logListWidget->verticalScrollBar();
  int oldMax = scrollBar->maximum();
  int oldValue = scrollBar->value();
  bool wasAtBottom = oldValue == oldMax;

  ui->logListWidget->addItem(logChange.message);

  if (logChange.trimmed) {
    delete ui->logListWidget->takeItem(0);
  }

  if (wasAtBottom) {
    ui->logListWidget->scrollToBottom();
  } else {
    scrollBar->setValue(oldValue - 1);
  }
}

void AppLogWidget::shutdown() { qInfo() << "AppLogWidget::shutdown()"; }