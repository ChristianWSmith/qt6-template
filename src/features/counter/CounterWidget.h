#ifndef COUNTERWIDGET_H
#define COUNTERWIDGET_H

#include "ui_CounterWidget.h"
#include <QPushButton>
#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {}
QT_END_NAMESPACE

class CounterWidget : public QWidget {
  Q_OBJECT

public:
  explicit CounterWidget(QWidget *parent = nullptr);

  ~CounterWidget();

  void displayCounter(int value);

signals:

  void incrementRequested();

private slots:

  void on_incrementButton_clicked();

private:
  Ui::CounterWidget *ui;
};

#endif