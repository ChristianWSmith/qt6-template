#pragma once

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
  void shutdown() override;

  AppLogWidget(const AppLogWidget &) = delete;
  AppLogWidget &operator=(const AppLogWidget &) = delete;
  AppLogWidget(AppLogWidget &&) = delete;
  AppLogWidget &operator=(AppLogWidget &&) = delete;

  QMetaObject::Connection connectClearRequested(QObject *receiver,
                                                const char *member) override;

  void clear() override;

public slots:
  void handleLogChanged(const LogDelta &logDelta) override;

signals:
  void clearRequested();

private slots:
  void on_clearButton_clicked();

private:
  Ui::AppLogWidget *ui;
};
