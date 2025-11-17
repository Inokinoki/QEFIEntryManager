find_package(QT NAMES Qt6 Qt5 COMPONENTS Test REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)

add_executable(test_dp_editor_view 
    test_dp_editor_view.cc 
    ../helpers.cpp
    ../qefivar/qefi.cpp
    ../qefivar/qefidpacpi.cpp
    ../qefivar/qefidphw.cpp
    ../qefivar/qefidpmedia.cpp
    ../qefivar/qefidpmessage.cpp
)
target_include_directories(test_dp_editor_view PRIVATE ${CMAKE_SOURCE_DIR}/qefivar)
target_link_libraries(test_dp_editor_view PRIVATE Qt${QT_VERSION_MAJOR}::Test Qt${QT_VERSION_MAJOR}::Widgets)
add_test(DPEditorViewTest test_dp_editor_view)

