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

# Embed resources.qrc itself (full manifest) so tests cannot drift from the
# production qrc list (E-004). AUTORCC is already ON at the root.
target_sources(UnitTests PRIVATE
    "${CMAKE_SOURCE_DIR}/resources/resources.qrc"
)

# Production-binary resource embedding regression check (Wave 3).
# Fails if resources.qrc content is not present in the shipped executable.
# Complementary runtime checks live in main.cpp --smoke-test (E-002).
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
