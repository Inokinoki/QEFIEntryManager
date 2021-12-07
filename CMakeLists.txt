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
        qefivar/qefi.cpp
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

# Find and link efivar/win32 API 
find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(EFIVAR efivar)
endif()

if(EFIVAR_FOUND)
    message("Use libefivar for EFI operations")
    include_directories(QEFIEntryManager ${EFIVAR_INCLUDE_DIRS})
    target_link_libraries(QEFIEntryManager PRIVATE ${EFIVAR_LIBRARIES})
elseif(WIN32)
    message("Use Windows API for EFI operations")
else()
    message(FATAL_ERROR "No EFI utility found, please make sure you have libefivar installed")
endif()

target_include_directories(QEFIEntryManager PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/qefivar)

# Make a CMAKE project config file in QEFI, the libefivar should be linked automatically
target_link_libraries(QEFIEntryManager PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
