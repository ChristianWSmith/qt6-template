#include "AppLogWidget.h"
#include <QScrollBar>
#include <fmt/core.h>

AppLogWidget::AppLogWidget(QWidget *parent)
    : IAppLogWidget(parent), ui(new Ui::AppLogWidget) {
  ui->setupUi(this);
  qDebug() << "AppLogWidget instantiated";
}

AppLogWidget::~AppLogWidget() { delete ui; }

void AppLogWidget::handleLogChanged(const LogDelta &logDelta) {
  QScrollBar *scrollBar = ui->logListWidget->verticalScrollBar();
  int oldMax = scrollBar->maximum();
  int oldValue = scrollBar->value();
  bool wasAtBottom = oldValue == oldMax;

  ui->logListWidget->addItem(logDelta.message);

  if (logDelta.trimmed) {
    delete ui->logListWidget->takeItem(0);
  }

  if (wasAtBottom) {
    ui->logListWidget->scrollToBottom();
  } else if (logDelta.trimmed) {
    scrollBar->setValue(oldValue - 1);
  }
}

void AppLogWidget::clear() { ui->logListWidget->clear(); }

void AppLogWidget::setLogMessages(const QVector<QString> &messages) {
  ui->logListWidget->addItems(messages);
  ui->logListWidget->scrollToBottom();
}

void AppLogWidget::on_clearButton_clicked() { emit clearRequested(); }

QMetaObject::Connection
AppLogWidget::connectClearRequested(QObject *receiver, const char *member) {
  return QObject::connect(this, SIGNAL(clearRequested()), receiver, member);
}

void AppLogWidget::shutdown() { qInfo() << "AppLogWidget::shutdown()"; }