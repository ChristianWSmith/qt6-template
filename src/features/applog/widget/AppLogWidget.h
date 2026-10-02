#pragma once

#include "ui_AppLogWidget.h"

#include <memory>
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

  void clear();
  // Full-state replacement (AUD-116): clears then adds. Name set* (not
  // display*) because the call promises replacement semantics, not an
  // incremental push. CounterWidget uses displayCounter for value pushes.
  void setLogMessages(const QStringList &messages);

signals:
  void clearRequested();

private slots:
  void on_clearButton_clicked();

private:
  std::unique_ptr<Ui::AppLogWidget> ui;
};
