set dotenv-load

QT_DIR := if env_var("QT_DIR") == "" { "/opt/Qt/6.8.3/gcc_64/lib/cmake" } else { env_var("QT_DIR") }
BUILD_TYPE := if env_var("BUILD_TYPE") == "" { "Release" } else { env_var("BUILD_TYPE") }
DCMAKE_TOOLCHAIN_FILE := env_var("DCMAKE_TOOLCHAIN_FILE")
BUILD_DIR := "build"

source:
    source /opt/emsdk/emsdk_env.sh

cmake:
    /opt/Qt/6.8.3/wasm_multithread/bin/qt-cmake -S . -B {{BUILD_DIR}} \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      -DCMAKE_TOOLCHAIN_FILE={{DCMAKE_TOOLCHAIN_FILE}} \
      -DCMAKE_PREFIX_PATH={{QT_DIR}} \
      -DCMAKE_BUILD_TYPE={{BUILD_TYPE}} \
      -G Ninja

build:
    ninja -C {{BUILD_DIR}}

run:
    cd {{BUILD_DIR}}/{{BUILD_TYPE}} && python3 -m http.server 8080

clean:
    rm -rf {{BUILD_DIR}}

b: build
c: cmake
r: run