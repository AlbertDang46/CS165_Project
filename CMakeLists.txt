cmake_minimum_required(VERSION 3.10.0)
project(TLSCache)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake/;${CMAKE_MODULE_PATH}")
set(LIBRESSL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/libressl_install")

find_package(LibreSSL REQUIRED)

add_executable(hello src/hello_world.cpp)

target_link_libraries(hello LibreSSL::TLS)


