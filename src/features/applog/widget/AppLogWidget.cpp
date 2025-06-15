#include "AppLogWidget.h"
#include <QScrollBar>
#include <fmt/core.h>

AppLogWidget::AppLogWidget(QWidget *parent)
    : IAppLogWidget(parent), ui(new Ui::AppLogWidget) {
  ui->setupUi(this);
  qDebug() << "AppLogWidget instantiated";
}

AppLogWidget::~AppLogWidget() { delete ui; }

void AppLogWidget::displayLogMessage(const QString &message) {

  ui->logTextEdit->append(message);

  QScrollBar *sb = ui->logTextEdit->verticalScrollBar();
  if (sb) {
    sb->setValue(sb->maximum());
  }
}
