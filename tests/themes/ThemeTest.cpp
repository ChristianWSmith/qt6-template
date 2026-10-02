// NOLINTBEGIN
#include "platform/theme/theme.hpp"

#include <QApplication>
#include <QFile>
#include <gtest/gtest.h>

// Theme/resources coverage for platform/theme/theme.hpp.
//
// Linux/macOS platform QSS files may be empty (Qt default styling); empty is
// valid and must not be treated as a failure. Resource-coverage assertions
// therefore target path registration / open success, never non-empty content.

// setTheme() mutates the process-global qApp stylesheet. Tests that change it
// must restore the prior value so later tests see a deterministic baseline.
class ScopedStyleSheetRestore {
public:
  ScopedStyleSheetRestore()
      : m_previous(qApp ? qApp->styleSheet() : QString()) {}
  ~ScopedStyleSheetRestore() {
    if (qApp != nullptr) {
      qApp->setStyleSheet(m_previous);
    }
  }

  ScopedStyleSheetRestore(const ScopedStyleSheetRestore &) = delete;
  ScopedStyleSheetRestore &operator=(const ScopedStyleSheetRestore &) = delete;

private:
  QString m_previous;
};

// Resolves the platform QSS path setTheme() loads on the current OS.
static QString platformQssPath() {
#ifdef Q_OS_WIN
  return isDarkMode() ? ":/styles/windows/dark.qss"
                      : ":/styles/windows/light.qss";
#elif defined(Q_OS_MACOS)
  return ":/styles/macos/base.qss";
#else
  return ":/styles/linux/base.qss";
#endif
}

// Resource embedding: UnitTests embeds style files via qt_add_resources so
// :/styles/ paths used by setTheme() are resolvable in the test binary.
TEST(ThemeTest, StyleResourcePathsAreRegistered) {
  EXPECT_TRUE(QFile::exists(platformQssPath()));
  EXPECT_TRUE(QFile::exists(":/styles/custom.qss"));
}

// setTheme() must run under the offscreen test platform without
// crashing. Re-entrant calls are safe (bootstrap is idempotent).
TEST(ThemeTest, SetThemeDoesNotCrash) {
  ScopedStyleSheetRestore restore;
  setTheme();
  setTheme();
  SUCCEED();
}

// After setTheme(), the application stylesheet reflects platform QSS +
// custom.qss. Empty concatenation (Linux/macOS default) is valid; the
// assertion locks that setTheme() actually overwrote any prior stylesheet.
TEST(ThemeTest, SetThemeAppliesStyleSheet) {
  ScopedStyleSheetRestore restore;
  qApp->setStyleSheet("QWidget { color: rgb(1, 2, 3); }");
  ASSERT_FALSE(qApp->styleSheet().isEmpty());

  setTheme();

  const QString expected =
      loadQSS(platformQssPath()) + loadQSS(":/styles/custom.qss");
  EXPECT_EQ(qApp->styleSheet(), expected);
}

TEST(ThemeTest, IsDarkModeIsStableWithinProcess) {
  const bool first = isDarkMode();
  const bool second = isDarkMode();
  EXPECT_EQ(first, second);
}

TEST(ThemeTest, LoadQssReturnsEmptyForMissingPath) {
  EXPECT_TRUE(loadQSS(":/styles/definitely/missing.qss").isEmpty());
}

// Existing resource paths must open even when content is empty (Linux/macOS
// Qt default styling). loadQSS returns "" for both missing and empty files,
// so open success is asserted via QFile directly.
TEST(ThemeTest, LoadQssOpensExistingPlatformResource) {
  QFile file(platformQssPath());
  ASSERT_TRUE(file.open(QFile::ReadOnly | QFile::Text));
}

TEST(ThemeTest, LoadQssOpensExistingCustomResource) {
  QFile file(":/styles/custom.qss");
  ASSERT_TRUE(file.open(QFile::ReadOnly | QFile::Text));
}

#include "ThemeTest.moc"

// NOLINTEND
