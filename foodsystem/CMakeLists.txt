# Minimum CMake required
cmake_minimum_required(VERSION 3.14.4)

# Project
project(grpc-foodsystem)

include_directories(${CMAKE_BINARY_DIR}/protos)

add_executable(foodsystem_client foodsystem_client.cc)
target_link_libraries(foodsystem_client PUBLIC foodsystem.grpc)

add_executable(foodsystem_server foodsystem_server.cc)
target_link_libraries(foodsystem_server PUBLIC foodsystem.grpc)