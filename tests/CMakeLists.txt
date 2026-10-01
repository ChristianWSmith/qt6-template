find_package(GTest REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Test)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS *.cpp)

# Single test binary; name hardcoded (no UT_NAME indirection).
add_executable(UnitTests
    ${TEST_SOURCES}
)

set_directory_properties(PROPERTIES QT_EXCLUDE_FROM_TRANSLATION ON)

# Autogen include dir comes from ${APP_NAME}_lib INTERFACE usage requirements.
target_include_directories(UnitTests PRIVATE
    ${CMAKE_SOURCE_DIR}/tests
    ${CMAKE_CURRENT_BINARY_DIR}
)

apply_app_metadata(UnitTests)

# Custom tests/main.cpp provides main() (QApplication + gtest init).
# Do not link gtest_main — that would hide the Qt application bootstrap.
target_link_libraries(UnitTests PRIVATE
    GTest::gtest
    ${APP_NAME}_lib
    Qt6::Core
    Qt6::Widgets
    Qt6::Test
    fmt::fmt
)

# Embed style resources into UnitTests so theme tests can access :/styles/
# QSS files used by platform/theme/theme.hpp. qt_standard_project_setup does
# not enable AUTORCC (only AUTOMOC/AUTOUIC), so a bare .qrc source is a no-op.
# Explicit qt_add_resources embeds only the style files — not exe packaging
# extras (icons, .rc) — via rcc into this target.
qt_add_resources(UnitTests "unit_test_theme_resources"
    PREFIX "/"
    BASE "${CMAKE_SOURCE_DIR}/resources"
    FILES
        "${CMAKE_SOURCE_DIR}/resources/styles/custom.qss"
        "${CMAKE_SOURCE_DIR}/resources/styles/linux/base.qss"
        "${CMAKE_SOURCE_DIR}/resources/styles/macos/base.qss"
        "${CMAKE_SOURCE_DIR}/resources/styles/windows/dark.qss"
        "${CMAKE_SOURCE_DIR}/resources/styles/windows/light.qss"
)

include(GoogleTest)
gtest_discover_tests(UnitTests
  DISCOVERY_TIMEOUT 30
)
