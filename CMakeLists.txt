cmake_minimum_required(VERSION 3.16)

project(MyApp VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)

set(TS_FILES MyApp_en_US.ts)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        ${TS_FILES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    set(APP_SOURCES
        ${PROJECT_SOURCES}
        resources.qrc
    )

    if(WIN32)
        list(APPEND APP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/MyApp.rc)
    elseif(APPLE)
        set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/icons/app_icon.icns PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources"
        )

        list(APPEND APP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/icons/app_icon.icns)

        set_target_properties(MyApp PROPERTIES
            MACOSX_BUNDLE_ICON_FILE app_icon.icns
        )
    endif()

    qt_add_executable(MyApp
        MANUAL_FINALIZATION
        ${APP_SOURCES}
    )

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    set(APP_SOURCES
        ${PROJECT_SOURCES}
        resources.qrc
    )

    if(WIN32)
        list(APPEND APP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/MyApp.rc)
    elseif(APPLE)
        set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/icons/app_icon.icns PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources"
        )

        list(APPEND APP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/icons/app_icon.icns)

        set_target_properties(MyApp PROPERTIES
            MACOSX_BUNDLE_ICON_FILE app_icon.icns
        )
    endif()

    if(ANDROID)
        add_library(MyApp SHARED
            ${APP_SOURCES}
        )
    else()
        add_executable(MyApp
            ${APP_SOURCES}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_link_libraries(MyApp PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

set(BUNDLE_ID_OPTION "")
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.MyApp)
endif()
set_target_properties(MyApp PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS MyApp
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(MyApp)
endif()
