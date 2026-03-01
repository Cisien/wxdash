cmake_minimum_required(VERSION 3.22)
project(wxdash LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable compile warnings
add_compile_options(-Wall -Wextra -Wpedantic)

find_package(Qt6 REQUIRED COMPONENTS Core Network Test Quick Qml Svg)
qt_standard_project_setup()

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
