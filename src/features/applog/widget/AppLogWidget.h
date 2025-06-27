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

  AppLogWidget(const AppLogWidget &) = delete;
  AppLogWidget &operator=(const AppLogWidget &) = delete;
  AppLogWidget(AppLogWidget &&) = delete;
  AppLogWidget &operator=(AppLogWidget &&) = delete;

  virtual void shutdown() override;

  virtual void clear();
  virtual void setLogMessages(const QVector<QString> &messages);
  virtual void handleLogChanged(const LogDelta &logDelta);

signals:
  void clearRequested();

private slots:
  void on_clearButton_clicked();

private:
  friend class AppLogTest;
  Ui::AppLogWidget *ui;
};
