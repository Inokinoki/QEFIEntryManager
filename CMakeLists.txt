cmake_minimum_required(VERSION 3.5)

set(QEFI_ENTRY_MANAGER_VERSION_MAJOR 0)
set(QEFI_ENTRY_MANAGER_VERSION_MINOR 4)
set(QEFI_ENTRY_MANAGER_VERSION_PATCH 1)
set(QEFI_ENTRY_MANAGER_VERSION ${QEFI_ENTRY_MANAGER_VERSION_MAJOR}.${QEFI_ENTRY_MANAGER_VERSION_MINOR}.${QEFI_ENTRY_MANAGER_VERSION_PATCH})

project(QEFIEntryManager LANGUAGES CXX VERSION ${QEFI_ENTRY_MANAGER_VERSION})

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Generate version header from template
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
    @ONLY
)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets Network LinguistTools REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Network REQUIRED)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        cli.cpp
        cli.h
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
        qefidpeditordialog.cpp
        qefidpeditordialog.h
        qefidpfields.cpp
        qefidpfields.h
        qefiloadoptioneditorview.cpp
        qefiloadoptioneditorview.h
        qefipartitionmanager.cpp
        qefipartitionmanager.h
        qefipartitionview.cpp
        qefipartitionview.h
        qefivar/qefi.cpp
        qefivar/qefidpacpi.cpp
        qefivar/qefidphw.cpp
        qefivar/qefidpmedia.cpp
        qefivar/qefidpmessage.cpp
)

# Add translations files here
set(TRANSLATIONS
    translations/app_en.ts
    translations/app_es.ts
    translations/app_fr.ts
    translations/app_pt.ts
    translations/app_zh.ts
)
set_source_files_properties(
    ${TRANSLATIONS} PROPERTIES
        # Set OUTPUT_LOCATION to the directory where the .qm files should be generated in Qt5
        OUTPUT_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/translations
        # Set TARGET_DIRECTORY to the directory where the .qm files should be generated in Qt6
        TARGET_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/translations
)

if(WIN32)
    list(PREPEND PROJECT_SOURCES WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "/MANIFESTUAC:\"level='requireAdministrator'\"")
endif()

# TODO: Set the linguist tools to optional
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    find_package(Qt6LinguistTools REQUIRED)
    qt_create_translation(QM_FILES ${TRANSLATIONS} ${CMAKE_CURRENT_SOURCE_DIR})
    add_executable(QEFIEntryManager
        ${PROJECT_SOURCES}
        translations/translations.qrc
    )
else()
    find_package(Qt5LinguistTools REQUIRED)
    qt5_create_translation(QM_FILES ${TRANSLATIONS} ${CMAKE_CURRENT_SOURCE_DIR})
    add_executable(QEFIEntryManager
        ${PROJECT_SOURCES}
        translations/translations.qrc
    )
endif()

if(APP_DATA_DUMMY_BACKEND)
    # Use the directory under QStandardPaths::AppDataLocation for test purpose
    message("Use dummy backend for EFI operations")
    add_definitions(-DEFIVAR_APP_DATA_DUMMY)
elseif(WIN32)
    message("Use Windows API for EFI operations")
    # Link with Windows libraries for partition management
    target_link_libraries(QEFIEntryManager PRIVATE setupapi cfgmgr32)
else()
    if(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
        # Link with FreeBSD system-level libefivar and geom
        # see source code of usr.sbin/efibootmgr/Makefile and usr.sbin/efivar/Makefile
        target_link_libraries(QEFIEntryManager PUBLIC efivar geom)
    endif()
	message("Use qefivar implementations for EFI operations")
endif()

if(USE_EFIVAR_OLD_API)
    add_definitions(-DEFIVAR_OLD_API)
endif()

target_include_directories(QEFIEntryManager PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/qefivar
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Make a CMAKE project config file in QEFI, the libefivar should be linked automatically
target_link_libraries(QEFIEntryManager PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network)

# Build qefibootmgr CLI-only executable
set(CLI_SOURCES
        cli.cpp
        cli.h
        qefientry.cpp
        qefientry.h
        qefientrystaticlist.cpp
        qefientrystaticlist.h
        qefivar/qefi.cpp
        qefivar/qefidpacpi.cpp
        qefivar/qefidphw.cpp
        qefivar/qefidpmedia.cpp
        qefivar/qefidpmessage.cpp
)

add_executable(qefibootmgr qefibootmgr.cpp ${CLI_SOURCES})

# qefibootmgr only needs Core (no Widgets/GUI)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)
target_link_libraries(qefibootmgr PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_include_directories(qefibootmgr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/qefivar)

# Set Windows admin elevation for CLI tools
if(WIN32)
    # qefibootmgr requires admin privileges on Windows
    set_target_properties(qefibootmgr PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
    target_link_options(qefibootmgr PRIVATE "/MANIFESTUAC:level='requireAdministrator'")

    # Build efibootmgr executable (for Windows compatibility)
    # On Windows, we create a separate executable instead of symlink
    # using the same source as qefibootmgr
    add_executable(efibootmgr qefibootmgr.cpp ${CLI_SOURCES})
    target_link_libraries(efibootmgr PRIVATE Qt${QT_VERSION_MAJOR}::Core)
    target_include_directories(efibootmgr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/qefivar)

    # efibootmgr also requires admin elevation
    set_target_properties(efibootmgr PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
    target_link_options(efibootmgr PRIVATE "/MANIFESTUAC:level='requireAdministrator'")
endif()

# Link FreeBSD libraries for CLI tools if needed
if(NOT APP_DATA_DUMMY_BACKEND AND NOT WIN32)
    if(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
        target_link_libraries(qefibootmgr PUBLIC efivar geom)
    endif()
endif()

# Install executables
if(WIN32)
    install(TARGETS QEFIEntryManager qefibootmgr efibootmgr
            RUNTIME DESTINATION bin
    )
else()
    install(TARGETS QEFIEntryManager qefibootmgr
            RUNTIME DESTINATION bin
    )
endif()
# For AppImage
include(GNUInstallDirs)
install(PROGRAMS qefientrymanager-launcher
        DESTINATION ${CMAKE_INSTALL_BINDIR}
)
# Generate the desktop file from the template
if(USE_PKEXEC_LAUNCHER)
    set(EXEC_COMMAND "qefientrymanager-launcher")
else()
    set(EXEC_COMMAND "QEFIEntryManager")
endif()
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/qefientrymanager.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/qefientrymanager.desktop @ONLY)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/qefientrymanager.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
)
install(FILES cc.inoki.qefientrymanager.png
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/96x96/apps
)

# Install man page for qefibootmgr/efibootmgr on Unix-like systems
if(UNIX)
    install(FILES qefibootmgr.8
            DESTINATION ${CMAKE_INSTALL_MANDIR}/man8
    )
endif()

# Create efibootmgr symlink to qefibootmgr on Unix-like systems
if(UNIX)
    install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink \
        qefibootmgr \
        \$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/efibootmgr)")
endif()

# Use CPack to generate the package(s)
if(ENABLE_PACKAGING)
set(CPACK_PACKAGE_NAME "qefientrymanager")
set(CPACK_PACKAGE_VERSION "${QEFI_ENTRY_MANAGER_VERSION}")
set(CPACK_PACKAGE_DISPLAY_NAME "EFI Entry Manager")
set(CPACK_PACKAGE_CONTACT "Inoki <veyx.shaw@gmail.com>")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/Inokinoki/QEFIEntryManager")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A userspace cross-platform EFI boot entry management GUI App based on Qt.")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_VENDOR "Inoki")
set(CPACK_PACKAGE_LICENSE "GPL-3.0")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}")
# For DEB
    set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "qt6-base")
    else()
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "qt5-default")
    endif()
# End DEB
# For RPM
    set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0")
    set(CPACK_RPM_PACKAGE_GROUP "Applications/System")
    set(CPACK_RPM_PACKAGE_REQUIRES "qt5-qtbase")
    if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
        set(CPACK_RPM_PACKAGE_DEPENDS "qt6-qtbase")
    else()
        set(CPACK_RPM_PACKAGE_DEPENDS "qt5-qtbase")
    endif()
# End RPM
# For NSIS
    set(CPACK_NSIS_PACKAGE_NAME ${CPACK_PACKAGE_NAME})
    set(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_DISPLAY_NAME} ${CPACK_PACKAGE_VERSION}")
    set(CPACK_NSIS_PACKAGE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")
    set(CPACK_NSIS_CONTACT ${CPACK_PACKAGE_CONTACT})
    set(CPACK_NSIS_URL ${CPACK_PACKAGE_HOMEPAGE_URL})
    # NSIS is not happy with the my "icon"
    # set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/cc.inoki.qefientrymanager.png")
    # set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/cc.inoki.qefientrymanager.png")
# End NSIS
# For FreeBSD
    set(CPACK_FREEBSD_PACKAGE_CATEGORIES "sysutils")
    set(CPACK_FREEBSD_PACKAGE_ORIGIN "sysutils/qefientrymanager")
    if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
        set(CPACK_FREEBSD_PACKAGE_DEPS "qt6")
    else()
        set(CPACK_FREEBSD_PACKAGE_DEPS "qt5")
    endif()
# End FreeBSD
include(CPack)
endif()
