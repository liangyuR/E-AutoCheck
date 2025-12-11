cmake_minimum_required(VERSION 3.21)
set(PROJECT_NAME "EAutoCheck")
project(${PROJECT_NAME} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Quick QuickControls2 Concurrent)
find_package(glog CONFIG REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(redis++ CONFIG REQUIRED)
find_package(PahoMqttCpp CONFIG REQUIRED)
find_package(amqpcpp CONFIG REQUIRED)
find_package(Libevent CONFIG REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(unofficial-mysql-connector-cpp CONFIG REQUIRED)

qt_standard_project_setup(REQUIRES 6.8)

file(GLOB_RECURSE PROJECT_SOURCES
    src/*.cpp
    src/client/*.cpp
    include/*.h
)

qt_add_executable(${PROJECT_NAME}
    ${PROJECT_SOURCES}
)

set_source_files_properties(qml/style/AppTheme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/style/AppFont.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/style/AppLayout.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(${PROJECT_NAME}
    URI GUI
    QML_FILES
        qml/Main.qml

        qml/page/Home.qml
        qml/page/Detial.qml
        qml/page/HistoryPage.qml
        
        qml/card/DeviceCard.qml
        qml/card/PileCard.qml

        qml/detail/PileDetail.qml

        qml/components/StatusRow.qml

        qml/style/AppLayout.qml
        qml/style/AppFont.qml
        qml/style/AppTheme.qml
    RESOURCES
        qml/qtquickcontrols2.conf

        assets/fonts/Roboto-Bold.ttf
        assets/fonts/Roboto-Medium.ttf
        assets/fonts/Roboto-Regular.ttf
        assets/fonts/MaterialIcons-Regular.ttf
)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core                   # Qt 6 核心库
    Qt6::Quick                  # Qt 6 QML/Quick 库
    Qt6::QuickControls2         # Qt 6 Quick Controls 2 扩展控件组件
    Qt6::Concurrent             # Qt 6 Concurrent 库
    glog::glog                  # Google 的日志库（glog）
    nlohmann_json::nlohmann_json # nlohmann-json，C++ 的 JSON 库
    yaml-cpp                    # yaml-cpp，YAML 解析库
    absl::base                  # Abseil 基础组件库
    absl::status                # Abseil Status
    absl::statusor              # Abseil StatusOr
    absl::str_format            # Abseil StrFormat
    absl::strings               # Abseil Strings
    redis++::redis++_static     # redis-plus-plus，C++ Redis 客户端（静态库）
    unofficial::mysql-connector-cpp::connector 
    unofficial::mysql-connector-cpp::connector-jdbc
    PahoMqttCpp::paho-mqttpp3   # Eclipse Paho MQTT C++3 客户端库
    amqpcpp                     # AMQP 协议 C++ 库，用于与 RabbitMQ 等通信
    event                       # libevent，事件通知库
)


install(TARGETS ${PROJECT_NAME}
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET ${PROJECT_NAME}
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})