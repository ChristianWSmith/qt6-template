cmake_minimum_required(VERSION 3.21)

project(${APP_NAME} VERSION ${APP_VERSION} LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_definitions(-DAPP_NAME="${APP_NAME}")
add_definitions(-DAPP_VERSION="${APP_VERSION}")
add_definitions(-DAPP_ID="${APP_ID}")

# Required for Qt6 features
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Only use Qt6
find_package(Qt6 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(fmt REQUIRED)
find_package(Vulkan REQUIRED)

file(GLOB_RECURSE TS_FILES CONFIGURE_DEPENDS resources/ts/*.ts)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.h src/*.ui)
file(GLOB_RECURSE RESOURCES CONFIGURE_DEPENDS resources/*.qrc)

set(PROJECT_SOURCES
    ${SOURCES}
    ${RESOURCES}
    ${TS_FILES}
)

# Handle platform-specific icon setup
if(WIN32)
    list(APPEND PROJECT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/resources/win_resources.rc)
elseif(APPLE)
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app_icon.icns PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    list(APPEND PROJECT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app_icon.icns)
endif()

qt_add_executable(${APP_NAME}
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

# macOS bundle metadata
if(APPLE)
    set_target_properties(${APP_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_ICON_FILE app_icon.icns
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE_GUI_IDENTIFIER ${APP_ID}
    )
elseif(WIN32)
    set_target_properties(${APP_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

target_link_libraries(${APP_NAME} PRIVATE Qt6::Widgets)
target_link_libraries(${APP_NAME} PRIVATE fmt::fmt)
target_link_libraries(${APP_NAME} PRIVATE Vulkan::Vulkan)

qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

include(GNUInstallDirs)
install(TARGETS ${APP_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(${APP_NAME})
