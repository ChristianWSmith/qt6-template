#pragma once

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleHints>

inline bool isDarkMode() {
    return QApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
}

inline QString loadQSS(const QString &path) {
  QFile file(path);
  if (!file.open(QFile::ReadOnly | QFile::Text)) {
    qWarning("Could not open QSS file: %s", qUtf8Printable(path));
    return "";
  }
  return QString::fromUtf8(file.readAll()).trimmed();
}

inline void setTheme() {
  QString filePath;

#ifdef Q_OS_WIN
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    filePath = isDarkMode() ? ":/styles/windows/dark.qss"
                            : ":/styles/windows/light.qss";
#elif defined(Q_OS_MACOS)
    filePath = ":/styles/macos/base.qss";
#else
    filePath = ":/styles/linux/base.qss";
#endif

  QString qss = loadQSS(filePath) + loadQSS(":/styles/custom.qss");
  qApp->setStyleSheet(qss);
}
