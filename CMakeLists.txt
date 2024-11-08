cmake_minimum_required(VERSION 3.5)

project(QEFIEntryManager LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets Network REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Network REQUIRED)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        qefientrydetailview.cpp
        qefientrydetailview.h
        qefientrydpdetailview.cpp
        qefientrydpdetailview.h
        qefientryview.cpp
        qefientryview.h
        qefientry.cpp
        qefientry.h
        qefientrystaticlist.cpp
        qefientrystaticlist.h
        helpers.cpp
        helpers.h
        qefidpeditorview.cpp
        qefidpeditorview.h
        qefiloadoptioneditorview.cpp
        qefiloadoptioneditorview.h
        qefivar/qefi.cpp
        qefivar/qefidpacpi.cpp
        qefivar/qefidphw.cpp
        qefivar/qefidpmedia.cpp
        qefivar/qefidpmessage.cpp
)

if(WIN32)
    list(PREPEND PROJECT_SOURCES WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "/MANIFESTUAC:\"level='requireAdministrator'\"")
endif()

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
    pkg_check_modules(EFIVAR efivar>=0.15)
    if(NOT DEFINED EFIVAR_FOUND)
        # Find and link efivar<0.15
        pkg_check_modules(EFIVAR efivar)
        if(EFIVAR_FOUND)
            # Consider it as an old lib (without mode and return non-zero)
            # TODO(Inoki): to check https://github.com/rhboot/efivar/commit/7bd2e309aeec210509d215c803a762725591ccc7
            add_definitions(-DEFIVAR_OLD_API)
        endif()
    endif()
endif()

if(APP_DATA_DUMMY_BACKEND)
    # Use the directory under QStandardPaths::AppDataLocation for test purpose
    message("Use dummy backend for EFI operations")
    add_definitions(-DEFIVAR_APP_DATA_DUMMY)
elseif(EFIVAR_FOUND)
    message("Use libefivar for EFI operations")
    include_directories(QEFIEntryManager ${EFIVAR_INCLUDE_DIRS})
    target_link_libraries(QEFIEntryManager PRIVATE ${EFIVAR_LIBRARIES})
elseif(WIN32)
    message("Use Windows API for EFI operations")
else()
    message(FATAL_ERROR "No EFI utility found, please make sure you have libefivar installed")
endif()

target_include_directories(QEFIEntryManager PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/qefivar)
if(PATCH_FREEBSD_EFIVAR)
    # Patch efivar 0.15 build for FreeBSD
    # TODO(Inoki): check 0.15 build
    add_definitions(-DEFIVAR_FREEBSD_PATCH)
    target_link_libraries(QEFIEntryManager PRIVATE geom)
endif()

# Make a CMAKE project config file in QEFI, the libefivar should be linked automatically
target_link_libraries(QEFIEntryManager PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network)

install(TARGETS QEFIEntryManager
        RUNTIME DESTINATION bin
)
# For AppImage
include(GNUInstallDirs)
install(PROGRAMS qefientrymanager-launcher
        DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(PROGRAMS qefientrymanager.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
)
install(FILES cc.inoki.qefientrymanager.png
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/96x96/apps
)
