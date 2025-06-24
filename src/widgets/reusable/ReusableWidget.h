#pragma once
#include "IReusableWidget.h"
#include "ui_ReusableWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui {}
QT_END_NAMESPACE

class ReusableWidget : public IReusableWidget {
  Q_OBJECT
  Q_INTERFACES(IReusableWidget)

public:
  explicit ReusableWidget(QWidget *parent = nullptr);
  ~ReusableWidget();

  ReusableWidget(const ReusableWidget &) = delete;
  ReusableWidget &operator=(const ReusableWidget &) = delete;
  ReusableWidget(ReusableWidget &&) = delete;
  ReusableWidget &operator=(ReusableWidget &&) = delete;

  void shutdown() override;

  // Implements IReusableWidget methods

signals:
  // Signals emitted by this concrete Widget

private slots:
  // Slots for UI events (auto-connected by Qt Designer)

private:
  Ui::ReusableWidget *ui;
};
