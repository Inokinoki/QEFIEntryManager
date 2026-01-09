cmake_minimum_required(VERSION 3.5)

set(QEFI_ENTRY_MANAGER_VERSION_MAJOR 0)
set(QEFI_ENTRY_MANAGER_VERSION_MINOR 4)
set(QEFI_ENTRY_MANAGER_VERSION_PATCH 1)
set(QEFI_ENTRY_MANAGER_VERSION ${QEFI_ENTRY_MANAGER_VERSION_MAJOR}.${QEFI_ENTRY_MANAGER_VERSION_MINOR}.${QEFI_ENTRY_MANAGER_VERSION_PATCH})

project(QEFIEntryManager LANGUAGES CXX VERSION ${QEFI_ENTRY_MANAGER_VERSION})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

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

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets Network LinguistTools Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Network Core REQUIRED)

set(COMMON_SOURCES
        qefientry.cpp
        qefientry.h
        qefientrystaticlist.cpp
        qefientrystaticlist.h
        qefipartitionmanager.cpp
        qefipartitionmanager.h
        qefivar/qefi.cpp
        qefivar/qefidpacpi.cpp
        qefivar/qefidphw.cpp
        qefivar/qefidpmedia.cpp
        qefivar/qefidpmessage.cpp
)

set(PROJECT_SOURCES ${COMMON_SOURCES}
                    main.cpp
                    mainwindow.cpp
                    mainwindow.h
                    qefientrydetailview.cpp
                    qefientrydetailview.h
                    qefientrydpdetailview.cpp
                    qefientrydpdetailview.h
                    qefientryview.cpp
                    qefientryview.h
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
                    qefipartitionview.cpp
                    qefipartitionview.h
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

    # Enable EFI partition disk image testing mode
    message("Enable EFI partition disk image testing mode - auto-generates test image")
    add_definitions(-DEFI_PARTITION_DISK_IMAGE)

    # Generate test EFI disk image during build
    set(TEST_EFI_IMAGE_DIR "${CMAKE_BINARY_DIR}/test_data")
    set(TEST_EFI_IMAGE "${TEST_EFI_IMAGE_DIR}/test_efi_disk.img")
    set(SOURCE_TEST_IMAGE "${CMAKE_SOURCE_DIR}/test_data/test_efi_disk.img")

    # Create test data directories
    file(MAKE_DIRECTORY "${TEST_EFI_IMAGE_DIR}")
    file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/test_data")

    # Add custom command to generate test image in source directory
    add_custom_command(
        OUTPUT "${SOURCE_TEST_IMAGE}"
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/create_test_efi_image.sh
        DEPENDS ${CMAKE_SOURCE_DIR}/scripts/create_test_efi_image.sh
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Generating test EFI disk image in source directory..."
    )

    # Add command to copy to build directory
    add_custom_command(
        OUTPUT "${TEST_EFI_IMAGE}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SOURCE_TEST_IMAGE}"
            "${TEST_EFI_IMAGE}"
        DEPENDS "${SOURCE_TEST_IMAGE}"
        COMMENT "Copying test EFI disk image to build directory..."
    )

    # Create a custom target for the test image
    add_custom_target(generate_test_efi_image ALL
        DEPENDS "${TEST_EFI_IMAGE}"
    )

    # Set the test image path as a compile definition
    target_compile_definitions(QEFIEntryManager PRIVATE
        TEST_EFI_IMAGE_PATH="${TEST_EFI_IMAGE}"
    )

    message("Test EFI image will be generated at: ${TEST_EFI_IMAGE}")
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
target_link_libraries(QEFIEntryManager PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Gui
)

install(TARGETS QEFIEntryManager
        RUNTIME DESTINATION bin
)

# The flag to build CLI utility
if(NOT BUILD_CLI_UTILITY)
    set(BUILD_CLI_UTILITY OFF)
endif()

# Build CLI utility if the flag is set
if(BUILD_CLI_UTILITY)
# Build qefibootmgr CLI-only executable
set(CLI_SOURCES ${COMMON_SOURCES}
                cli.cpp
                cli.h
)
add_executable(qefibootmgr qefibootmgr.cpp ${CLI_SOURCES})
target_link_libraries(qefibootmgr PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_include_directories(qefibootmgr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/qefivar)

# Set Windows admin elevation for CLI tools
if(WIN32)
    # qefibootmgr requires admin privileges on Windows
    set_target_properties(qefibootmgr PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
    target_link_options(qefibootmgr PRIVATE "/MANIFESTUAC:level='requireAdministrator'")
    # Link Windows libraries for partition management (qefipartitionmanager.cpp uses Windows APIs)
    target_link_libraries(qefibootmgr PRIVATE setupapi cfgmgr32)

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
    # Link Windows libraries for partition management (qefipartitionmanager.cpp uses Windows APIs)
    target_link_libraries(efibootmgr PRIVATE setupapi cfgmgr32)
endif()

# Link FreeBSD libraries for CLI tools if needed
if(NOT APP_DATA_DUMMY_BACKEND AND NOT WIN32)
    if(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
        target_link_libraries(qefibootmgr PUBLIC efivar geom)
    endif()
endif()

# Install executables
if(WIN32)
    install(TARGETS qefibootmgr efibootmgr
            RUNTIME DESTINATION bin
    )
else()
    install(TARGETS qefibootmgr 
            RUNTIME DESTINATION bin
    )
endif()


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
