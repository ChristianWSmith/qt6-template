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
# QSS files used by platform/theme/theme.hpp. The production executable embeds
# resources.qrc via CMAKE_AUTORCC ON (root CMakeLists.txt). UnitTests does not
# link the exe, so this target embeds style files independently via
# qt_add_resources (belt-and-suspenders for the test binary only).
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

# Production-binary resource embedding regression check (Wave 3).
# Fails if resources.qrc content is not present in the shipped executable.
add_test(NAME ProductionResourcesEmbedded
    COMMAND ${CMAKE_COMMAND}
        -DAPP_EXE=$<TARGET_FILE:${APP_NAME}>
        -P "${CMAKE_SOURCE_DIR}/cmake/VerifyProductionResources.cmake"
)

# Architecture boundary heuristic checks (also run in scripts/build.sh).
# Available on Unix CI/dev hosts; Windows may lack bash in PATH for direct ctest.
if(UNIX)
  add_test(NAME ArchitectureBoundaries
      COMMAND bash "${CMAKE_SOURCE_DIR}/scripts/check-architecture-boundaries.sh")
endif()

include(GoogleTest)
gtest_discover_tests(UnitTests
  DISCOVERY_TIMEOUT 30
)
