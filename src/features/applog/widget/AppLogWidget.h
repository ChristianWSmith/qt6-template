#ifndef APPLOGWIDGET_H
#define APPLOGWIDGET_H

#include "IAppLogWidget.h"
#include "ui_AppLogWidget.h"
#include <QTextEdit>

QT_BEGIN_NAMESPACE
namespace Ui {
class AppLogWidget;
}
QT_END_NAMESPACE

class AppLogWidget : public IAppLogWidget {
  Q_OBJECT
  Q_INTERFACES(IAppLogWidget)

public:
  explicit AppLogWidget(QWidget *parent = nullptr);
  ~AppLogWidget();

public slots:
  void displayLogMessage(const QString &message) override;

signals:

private slots:

private:
  Ui::AppLogWidget *ui;
};

#endif