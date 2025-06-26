#pragma once

#include "ui_AppLogWidget.h"
#include <QTextEdit>

#include "../../../core/IWidget.h"
#include "../applogcommon.h"
#include <QString>
#include <QWidget>
#include <QtPlugin>

QT_BEGIN_NAMESPACE
namespace Ui {}
QT_END_NAMESPACE

class AppLogWidget : public QWidget, public IWidget {
  Q_OBJECT

public:
  explicit AppLogWidget(QWidget *parent = nullptr);
  ~AppLogWidget();
  void shutdown() override;

  AppLogWidget(const AppLogWidget &) = delete;
  AppLogWidget &operator=(const AppLogWidget &) = delete;
  AppLogWidget(AppLogWidget &&) = delete;
  AppLogWidget &operator=(AppLogWidget &&) = delete;

  void clear();
  void setLogMessages(const QVector<QString> &messages);

public slots:
  void handleLogChanged(const LogDelta &logDelta);

signals:
  void clearRequested();

private slots:
  void on_clearButton_clicked();

private:
  Ui::AppLogWidget *ui;
};
