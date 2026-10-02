#pragma once

#include "../../logging/logging.h"

#include <QApplication>
#include <QFile>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleHints>

// App-global bootstrap theming. Call once after window construction.
// Empty platform QSS files mean Qt default styling; custom.qss is the
// primary application override extension point.
// Windows also forces the Fusion style before applying platform QSS.
//
// colorScheme() is sampled once at setTheme() call time. There is no
// QStyleHints::colorSchemeChanged connection — runtime OS dark/light
// switches do not re-apply QSS until the app restarts. Linux/macOS paths
// never consult colorScheme() (static platform QSS only).

inline bool isDarkMode() {
    return QApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
}

inline QString loadQSS(const QString &path) {
  QFile file(path);
  if (!file.open(QFile::ReadOnly | QFile::Text)) {
    qCWarning(appMain) << "Could not open QSS file:" << path;
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
