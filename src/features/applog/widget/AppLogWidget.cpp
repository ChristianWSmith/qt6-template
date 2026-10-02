#include "AppLogWidget.h"
#include "../../../logging/logging.h"

AppLogWidget::AppLogWidget(QWidget *parent)
    : QWidget(parent), ui(std::make_unique<Ui::AppLogWidget>()) {
  ui->setupUi(this);
  qCDebug(appFeature) << "AppLogWidget instantiated";
}

AppLogWidget::~AppLogWidget() = default;

void AppLogWidget::clear() { ui->logListWidget->clear(); }

void AppLogWidget::setLogMessages(const QStringList &messages) {
  ui->logListWidget->clear();
  ui->logListWidget->addItems(messages);
  ui->logListWidget->scrollToBottom();
}

void AppLogWidget::on_clearButton_clicked() { emit clearRequested(); }
