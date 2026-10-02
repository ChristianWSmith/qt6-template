#pragma once

#include "ui_AppLogWidget.h"

#include "../applogcommon.h"
#include <memory>
#include <QString>
#include <QStringList>
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

  // View API convention: display* for presenter→view pushes (see AGENTS.md).
  void clear();
  void displayLogMessages(const QStringList &messages);
  void displayLogChanged(const LogDelta &logDelta);

signals:
  void clearRequested();

private slots:
  void on_clearButton_clicked();

private:
  std::unique_ptr<Ui::AppLogWidget> ui;
};
