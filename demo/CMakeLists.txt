cmake_minimum_required(VERSION 2.8.12)

project(rect_pack_demo)

add_executable(
    demo
    args.cpp
    bitmap_canvas.cpp
    main.cpp
    palette.cpp
    svg_canvas.cpp
)

target_compile_options(demo
    PRIVATE -std=c++11 -Wall -Wextra -pedantic)
set_target_properties(demo
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(demo PRIVATE ..)

find_package(PNG REQUIRED)
target_include_directories(demo PRIVATE ${PNG_INCLUDE_DIRS})
target_link_libraries(demo ${PNG_LIBRARIES})
