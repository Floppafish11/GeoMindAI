cmake_minimum_required(VERSION 3.21)
project(GeoMindAI VERSION 0.1.0)

add_library(${PROJECT_NAME} SHARED
    src/main.cpp
)

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)
setup_geode_mod(${PROJECT_NAME})
