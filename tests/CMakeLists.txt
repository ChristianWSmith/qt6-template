find_package(GTest REQUIRED)
find_package(fmt REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets Test)

enable_testing()


file(GLOB_RECURSE UI_FILES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.ui)
qt6_wrap_ui(GENERATED_UI_HEADERS ${UI_FILES})

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS *.cpp)

add_executable(${UT_NAME}
    ${TEST_SOURCES}
    ${GENERATED_UI_HEADERS}
)

set_directory_properties(PROPERTIES QT_EXCLUDE_FROM_TRANSLATION ON)

target_include_directories(${UT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(${UT_NAME} PRIVATE
    GTest::gtest_main
    GTest::gmock
    ${APP_NAME}_lib
    Qt6::Core
    Qt6::Widgets
    Qt6::Test
    fmt::fmt
)

include(GoogleTest)
gtest_discover_tests(${UT_NAME}
  DISCOVERY_TIMEOUT 30
)
