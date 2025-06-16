find_package(GTest REQUIRED)
find_package(fmt REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Test)
enable_testing()

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS *.cpp)

add_executable(${UT_NAME} ${TEST_SOURCES})

target_link_libraries(${UT_NAME} PRIVATE
    GTest::gtest_main
    GTest::gmock
    ${APP_NAME}_lib
    Qt6::Core
    Qt6::Widgets
    Qt6::Test
    fmt::fmt
)

target_include_directories(${UT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

set_target_properties(${UT_NAME} PROPERTIES
    AUTOMOC ON
)

include(GoogleTest)
gtest_discover_tests(${UT_NAME})
