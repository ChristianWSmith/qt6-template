#pragma once

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QOperatingSystemVersion>
#include <QPalette>
#include <QStyle>
#include <QStyleFactory>

#ifdef Q_OS_WIN
#include <windows.h>

bool isWindowsDarkModeEnabled() {
  DWORD value = 1;
  DWORD dataSize = sizeof(value);
  if (RegGetValueW(
          HKEY_CURRENT_USER,
          L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
          L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &value,
          &dataSize) != ERROR_SUCCESS) {
    return false; // Assume light mode if key can't be read
  }
  return value == 0;
}
#else
bool isWindowsDarkModeEnabled() { return false; };
#endif

QString loadQSS(const QString &path) {
  QFile file(path);
  if (!file.open(QFile::ReadOnly | QFile::Text)) {
    qWarning("Could not open QSS file: %s", qUtf8Printable(path));
    return "";
  }
  return QString::fromUtf8(file.readAll()).trimmed();
}

void setTheme() {
  QString filePath;

  if (QOperatingSystemVersion::currentType() ==
      QOperatingSystemVersion::Windows) {
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    filePath = isWindowsDarkModeEnabled() ? ":/styles/windows/dark.qss"
                                          : ":/styles/windows/light.qss";
  } else if (QOperatingSystemVersion::currentType() ==
             QOperatingSystemVersion::MacOS) {
    filePath = ":/styles/macos/base.qss";
  } else {
    filePath = ":/styles/linux/base.qss";
  }

  QString qss = loadQSS(filePath) + loadQSS(":/styles/custom.qss");
  qApp->setStyleSheet(qss);
}
