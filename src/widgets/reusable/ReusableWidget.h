#pragma once
#include "../../core/IWidget.h"
#include "ui_ReusableWidget.h"
#include <QWidget>
#include <QtPlugin>

QT_BEGIN_NAMESPACE
namespace Ui {}
QT_END_NAMESPACE

class ReusableWidget : public QWidget, public IWidget {
  Q_OBJECT

public:
  explicit ReusableWidget(QWidget *parent = nullptr);
  ~ReusableWidget();

  ReusableWidget(const ReusableWidget &) = delete;
  ReusableWidget &operator=(const ReusableWidget &) = delete;
  ReusableWidget(ReusableWidget &&) = delete;
  ReusableWidget &operator=(ReusableWidget &&) = delete;

  // IWidget
  void shutdown() override;

signals:
  // Signals emitted by this Widget to be connected to Presenter Slots

private slots:
  // Slots for UI events (auto-connected by Qt Designer)

private:
  Ui::ReusableWidget *ui;
};
