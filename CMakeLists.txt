cmake_minimum_required(VERSION 3.10)
project(cGame)

# Set C and C++ standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Optimization
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

# Create build directory if it doesn't exist
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/build)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/lib)

# Source files
set(C_SOURCES
    lib/cursed_drawing.c
    lib/txt_reader.c
    lib/txt_writer.c
)

set(CPP_SOURCES
    lib/code_reading.cpp
    lib/cmd_logic.cpp
    lib/menu_logic.cpp
    lib/game_elements.cpp
    lib/irc_logic.cpp
    src/main.cpp
)

# Compile object files
add_library(c_objects OBJECT ${C_SOURCES})
add_library(cpp_objects OBJECT ${CPP_SOURCES})

# Output executable to the project root
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(cGame.x86_64
    $<TARGET_OBJECTS:c_objects>
    $<TARGET_OBJECTS:cpp_objects>
)

# External libraries
target_link_libraries(cGame.x86_64
    crypto
    ssl
    m
    ncurses
    tcc
)
