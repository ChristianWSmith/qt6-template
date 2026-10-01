#pragma once
#include "ui_ReusableWidget.h"
#include <memory>
#include <QWidget>

// Intentional teaching example of the standalone-widget convention
// (see scripts/generate.sh widget mode and AGENTS.md). Not instantiated
// by the sample application.

QT_BEGIN_NAMESPACE
namespace Ui {}
QT_END_NAMESPACE

class ReusableWidget : public QWidget {
  Q_OBJECT

public:
  explicit ReusableWidget(QWidget *parent = nullptr);
  ~ReusableWidget();

  ReusableWidget(const ReusableWidget &) = delete;
  ReusableWidget &operator=(const ReusableWidget &) = delete;
  ReusableWidget(ReusableWidget &&) = delete;
  ReusableWidget &operator=(ReusableWidget &&) = delete;

signals:
  // Signals emitted by this Widget to be connected to Presenter Slots

private slots:
  // Slots for UI events (auto-connected by Qt Designer)

private:
  std::unique_ptr<Ui::ReusableWidget> ui;
};
