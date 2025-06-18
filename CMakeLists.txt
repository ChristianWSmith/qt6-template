cmake_minimum_required(VERSION 3.21)

project(${APP_NAME} VERSION ${APP_VERSION} LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_definitions(-DAPP_NAME="${APP_NAME}")
add_definitions(-DAPP_DESCRIPTION="${APP_DESCRIPTION}")
add_definitions(-DAPP_VERSION="${APP_VERSION}")
add_definitions(-DAPP_ID="${APP_ID}")

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(fmt REQUIRED)
find_package(Vulkan REQUIRED)
find_package(cxxopts REQUIRED)
find_package(ZLIB REQUIRED)

qt_standard_project_setup(
    I18N_TRANSLATED_LANGUAGES en es de fr
)

file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.h src/*.hpp src/*.ui)

file(GLOB_RECURSE RESOURCES CONFIGURE_DEPENDS resources/*.qrc)

add_library(${APP_NAME}_lib STATIC
    ${LIB_SOURCES}
)

target_link_libraries(${APP_NAME}_lib PRIVATE
    Qt6::Widgets
    fmt::fmt
    Vulkan::Vulkan
    cxxopts::cxxopts
    ZLIB::ZLIB
)

target_compile_definitions(${APP_NAME}_lib PRIVATE
    APP_NAME="${APP_NAME}"
    APP_DESCRIPTION="${APP_DESCRIPTION}"
    APP_VERSION="${APP_VERSION}"
    APP_ID="${APP_ID}"
)

if(APPLE)
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app_icon.icns PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    list(APPEND RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app_icon.icns)
elseif(WIN32)
    list(APPEND RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/resources/win_resources.rc)
endif()

qt_add_executable(${APP_NAME}
    MANUAL_FINALIZATION
    ${RESOURCES}
)

qt_add_translations(${APP_NAME}
    TS_FILE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources/i18n"
)

target_link_libraries(${APP_NAME} PRIVATE ${APP_NAME}_lib)

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

include(GNUInstallDirs)

install(TARGETS ${APP_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(${APP_NAME})

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

