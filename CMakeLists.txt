cmake_minimum_required(VERSION 3.21)
project(wasm_qml_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Quick QuickControls2)

qt_standard_project_setup(REQUIRES 6.8)
qt_add_executable(wasm_qml_demo
    src/main.cpp

    src/http/http_client.cpp
    src/http/http_client.h
)

qt_add_qml_module(wasm_qml_demo
    URI EAutoCheckUI
    VERSION 1.0
    QML_FILES
        qml/Main.qml        
)
    
target_link_libraries(wasm_qml_demo
    PRIVATE
        Qt6::Core
        Qt6::Quick
        Qt6::QuickControls2
)
