#pragma once

#include "ui_AppLogWidget.h"
#include <QTextEdit>

#include "../applogcommon.h"
#include <QString>
#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {}
QT_END_NAMESPACE

class AppLogWidget : public QWidget {
  Q_OBJECT

public:
  explicit AppLogWidget(QWidget *parent = nullptr);
  ~AppLogWidget();

  AppLogWidget(const AppLogWidget &) = delete;
  AppLogWidget &operator=(const AppLogWidget &) = delete;
  AppLogWidget(AppLogWidget &&) = delete;
  AppLogWidget &operator=(AppLogWidget &&) = delete;

  void clear();
  void setLogMessages(const QVector<QString> &messages);
  void handleLogChanged(const LogDelta &logDelta);

signals:
  void clearRequested();

private slots:
  void on_clearButton_clicked();

private:
  friend class AppLogTest;
  Ui::AppLogWidget *ui;
};
