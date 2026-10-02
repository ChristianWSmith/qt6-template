#include "AppLogWidget.h"
#include "../../../logging/logging.h"
#include <QScrollBar>

AppLogWidget::AppLogWidget(QWidget *parent)
    : QWidget(parent), ui(std::make_unique<Ui::AppLogWidget>()) {
  ui->setupUi(this);
  qCDebug(appFeature) << "AppLogWidget instantiated";
}

AppLogWidget::~AppLogWidget() = default;

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

void AppLogWidget::setLogMessages(const QStringList &messages) {
  ui->logListWidget->addItems(messages);
  ui->logListWidget->scrollToBottom();
}

void AppLogWidget::on_clearButton_clicked() { emit clearRequested(); }