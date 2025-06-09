cmake_minimum_required(VERSION 3.21)

project(MyApp VERSION 0.1 LANGUAGES CXX)

# Required for Qt6 features
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Only use Qt6
find_package(Qt6 REQUIRED COMPONENTS Widgets LinguistTools)

set(TS_FILES MyApp_en_US.ts)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    resources.qrc
    ${TS_FILES}
)

# Handle platform-specific icon setup
if(WIN32)
    list(APPEND PROJECT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/MyApp.rc)
elseif(APPLE)
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/icons/app_icon.icns PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    list(APPEND PROJECT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/icons/app_icon.icns)
endif()

qt_add_executable(MyApp
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

# macOS bundle metadata
if(APPLE)
    set_target_properties(MyApp PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_ICON_FILE app_icon.icns
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE_GUI_IDENTIFIER com.example.MyApp
    )
endif()

# Windows-specific metadata
if(WIN32)
    set_target_properties(MyApp PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

target_link_libraries(MyApp PRIVATE Qt6::Widgets)

qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

include(GNUInstallDirs)
install(TARGETS MyApp
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(MyApp)
