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

file(GLOB_RECURSE TS_FILES CONFIGURE_DEPENDS resources/i18n/*.ts)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.h src/*.ui src/*.hpp)
file(GLOB_RECURSE RESOURCES CONFIGURE_DEPENDS resources/*.qrc)

qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

set(COPIED_QM_FILES "")
foreach(qm_file IN LISTS QM_FILES)
    get_filename_component(qm_basename ${qm_file} NAME)
    set(DEST_QM_PATH ${CMAKE_CURRENT_SOURCE_DIR}/resources/i18n/${qm_basename})
    add_custom_command(
        OUTPUT ${DEST_QM_PATH}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${qm_file} ${DEST_QM_PATH}
        DEPENDS ${qm_file}
    )
    list(APPEND COPIED_QM_FILES ${DEST_QM_PATH})
endforeach()

foreach(qrc_file IN LISTS RESOURCES)
    set_source_files_properties(${qrc_file} PROPERTIES
        AUTORCC_DEPENDS "${COPIED_QM_FILES}"
    )
endforeach()

set(PROJECT_SOURCES
    ${SOURCES}
    ${RESOURCES}
)

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
target_link_libraries(${APP_NAME} PRIVATE cxxopts::cxxopts)

include(GNUInstallDirs)

install(FILES ${QM_FILES}
        DESTINATION ${CMAKE_INSTALL_BINDIR}/i18n
        COMPONENT Runtime
)

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
