cmake_minimum_required(VERSION 3.21)
project(EAutoCheck LANGUAGES CXX)
set(PROJECT_NAME "EAutoCheck")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Quick QuickControls2)
find_package(glog CONFIG REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(redis++ CONFIG REQUIRED)
find_package(PahoMqttCpp CONFIG REQUIRED)
find_package(amqpcpp CONFIG REQUIRED)
find_package(Libevent CONFIG REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(ZLIB REQUIRED)
find_package(unofficial-mysql-connector-cpp CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)

qt_standard_project_setup(REQUIRES 6.8)

file(GLOB_RECURSE PROJECT_SOURCES
    src/*.cpp
    src/client/*.cpp
    include/*.h
)

qt_add_executable(${PROJECT_NAME}
    ${PROJECT_SOURCES}
)

qt_add_qml_module(${PROJECT_NAME}
    URI GUI
    QML_FILES
        qml/Main.qml        
)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Quick
    Qt6::QuickControls2
    glog::glog
    nlohmann_json::nlohmann_json
    redis++::redis++_static
    PahoMqttCpp::paho-mqttpp3
    PahoMqttCpp::paho-mqttpp3-static
    amqpcpp
    event
    yaml-cpp
    ZLIB::ZLIB
    unofficial::mysql-connector-cpp::connector
    absl::base
)
