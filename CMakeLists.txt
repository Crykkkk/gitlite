cmake_minimum_required(VERSION 3.10)

project(gitlite)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

file(GLOB SRC_FILES "src/*.cpp")

list(APPEND SRC_FILES "main.cpp")

add_executable(gitlite ${SRC_FILES})