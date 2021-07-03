cmake_minimum_required(VERSION 3.5)

project(QEFIEntryManager LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

find_package(QEFI REQUIRED)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        qefientryview.cpp
        qefientryview.h
        qefientryrebootview.cpp
        qefientryrebootview.h
        qefientry.cpp
        qefientry.h
        qefientrystaticlist.cpp
        qefientrystaticlist.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(QEFIEntryManager
        ${PROJECT_SOURCES}
    )
else()
    add_executable(QEFIEntryManager
        ${PROJECT_SOURCES}
    )
endif()

# Make a CMAKE project config file in QEFI, the libefivar should be linked automatically
target_link_libraries(QEFIEntryManager PRIVATE Qt${QT_VERSION_MAJOR}::Widgets QEFI)
