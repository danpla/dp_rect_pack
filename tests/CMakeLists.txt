cmake_minimum_required(VERSION 2.8.12)

project(rect_pack_tests)

add_executable(
    tests
    main.cpp
)

target_compile_options(tests
    PRIVATE -std=c++98 -Wall -Wextra -pedantic)

target_include_directories(tests PRIVATE ..)
