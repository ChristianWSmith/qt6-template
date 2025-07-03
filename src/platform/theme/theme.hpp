#pragma once

#ifdef Q_OS_WIN
#include <QApplication>
#include <QFile>
#include <QPalette>
#include <QStyle>
#include <QStyleFactory>
#include <windows.h>

bool isDarkModeEnabled() {
  DWORD value = 1;
  DWORD dataSize = sizeof(value);
  RegGetValueW(
      HKEY_CURRENT_USER,
      L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
      L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &value, &dataSize);
  return value == 0;
}
#endif

void setTheme() {
#ifdef Q_OS_WIN
  QApplication::setStyle(QStyleFactory::create("Fusion"));

  QString filePath;
  if (isDarkModeEnabled()) {
    filePath = ":/styles/windows_dark.qss";
  } else {
    filePath = ":/styles/windows_light.qss";
  }

  QFile styleFile(filePath);
  if (styleFile.open(QFile::ReadOnly)) {
    QString styleSheet = QLatin1String(styleFile.readAll());
    qApp->setStyleSheet(styleSheet);
  }
#endif
}