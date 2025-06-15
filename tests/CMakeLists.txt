find_package(GTest REQUIRED)
find_package(fmt REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Test)
enable_testing()

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS *.cpp)

add_executable(unit_tests ${TEST_SOURCES})

target_link_libraries(unit_tests PRIVATE
    GTest::gtest_main
    GTest::gmock
    ${APP_NAME}_lib
    Qt6::Core
    Qt6::Widgets
    Qt6::Test
    fmt::fmt
)

target_include_directories(unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

set_target_properties(unit_tests PROPERTIES
    AUTOMOC ON
)

include(GoogleTest)
gtest_discover_tests(unit_tests)
