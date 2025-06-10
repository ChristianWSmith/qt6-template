#include "app.h"
#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <fmt/core.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    
    setWindowTitle(APP_NAME);

    connect(ui->incrementButton, &QPushButton::clicked, this, &MainWindow::incrementCounter);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::incrementCounter() {
    counter++;
    ui->counterLabel->setText(QString::number(counter));
    fmt::print("Counter incremented to: {}\n", counter);
}
