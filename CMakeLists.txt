cmake_minimum_required(VERSION 3.10)
project(minsql)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_C_FLAGS "-g")

include_directories("include/")
set(SOURCE_FILE "./src/main.c")

add_executable(${PROJECT_NAME} ${SOURCE_FILE})