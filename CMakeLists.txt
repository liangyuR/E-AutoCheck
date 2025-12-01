cmake_minimum_required(VERSION 3.21)
set(PROJECT_NAME "EAutoCheck")
project(${PROJECT_NAME} LANGUAGES CXX)

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

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
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
    Qt6::Core                   # Qt 6 核心库
    Qt6::Quick                  # Qt 6 QML/Quick 库
    Qt6::QuickControls2         # Qt 6 Quick Controls 2 扩展控件组件
    glog::glog                  # Google 的日志库（glog）
    nlohmann_json::nlohmann_json # nlohmann-json，C++ 的 JSON 库
    yaml-cpp                    # yaml-cpp，YAML 解析库
    absl::base                  # Abseil 基础组件库
    redis++::redis++_static     # redis-plus-plus，C++ Redis 客户端（静态库）
    unofficial::mysql-connector-cpp::connector   # MySQL C++ 连接器
    PahoMqttCpp::paho-mqttpp3   # Eclipse Paho MQTT C++3 客户端库
    amqpcpp                     # AMQP 协议 C++ 库，用于与 RabbitMQ 等通信
    event                       # libevent，事件通知库
)
