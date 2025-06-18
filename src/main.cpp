#include "appmainwindow/AppMainWindow.h"

#include "logging/logging.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QIcon>
#include <QLocale>
#include <QTranslator>
#include <cxxopts.hpp>
#include <fmt/core.h>
#include <iostream>
#include <qdebug.h>
#include <zlib.h>

void testZlibCompression() {
  std::string input = "Hello, zlib!";
  std::string compressed_output;
  std::string decompressed_output;

  // A very basic zlib compression (not robust error handling for a real app)
  uLong sourceLen = input.length();
  uLong destLen = compressBound(sourceLen); // Max size for compressed data
  std::vector<Bytef> compressed_data(destLen);

  int result = compress(compressed_data.data(), &destLen,
                        (const Bytef *)input.data(), sourceLen);

  if (result == Z_OK) {
    compressed_output.assign(reinterpret_cast<char *>(compressed_data.data()),
                             destLen);
    std::cout << "Zlib: Original size: " << sourceLen
              << ", Compressed size: " << destLen << std::endl;

    uLong decompressedLen = sourceLen; // Original size for decompression
    std::vector<Bytef> decompressed_data(decompressedLen);
    int decompress_result =
        uncompress(decompressed_data.data(), &decompressedLen,
                   compressed_data.data(), destLen);

    if (decompress_result == Z_OK) {
      decompressed_output.assign(
          reinterpret_cast<char *>(decompressed_data.data()), decompressedLen);
      std::cout << "Zlib: Decompressed: " << decompressed_output << std::endl;
    } else {
      std::cerr << "Zlib: Decompression failed: " << decompress_result
                << std::endl;
    }

  } else {
    std::cerr << "Zlib: Compression failed: " << result << std::endl;
  }
}

int main(int argc, char *argv[]) {
  testZlibCompression();
  cxxopts::Options options(APP_NAME, APP_DESCRIPTION);
  options.add_options()("l, log", "Log level (debug, info, warn, error, none)",
                        cxxopts::value<std::string>()->default_value("info"))(
      "h, help", "Print help")(
      "smoke-test", "Run in smoke test mode (exits immediately after setup)");
  cxxopts::ParseResult result = options.parse(argc, argv);

  if (result.count("help")) {
    fprintf(stdout, "%s\n", options.help().c_str());
    return 0;
  }

  setLogLevel(parseLogLevel(result["log"].as<std::string>()));
  qInstallMessageHandler(messageHandler);

  qInfo() << fmt::format("Hello from {} {}!", APP_NAME, APP_VERSION).c_str();
  QApplication a(argc, argv);

  a.setWindowIcon(QIcon(":/icons/app_icon.png"));

  QGuiApplication::setDesktopFileName(APP_ID);

  QLocale locale = QLocale::system();
  QString langCode = locale.name();
  QString baseLang = langCode.section('_', 0, 0);

  QTranslator translator;
  if (translator.load(QString("%1_%2.qm").arg(APP_NAME).arg(baseLang),
                      ":/i18n")) {
    QCoreApplication::installTranslator(&translator);
  } else {
    qWarning() << "Failed to load translation for" << baseLang;
  }

  AppMainWindow appMainWindow;

  if (result.count("smoke-test")) {
    qInfo() << "Smoke test successful: Application initialized and exiting.";
    return 0;
  }

  appMainWindow.show();

  return a.exec();
}