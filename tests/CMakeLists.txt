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

include(GoogleTest)
gtest_discover_tests(UnitTests
  DISCOVERY_TIMEOUT 30
)
