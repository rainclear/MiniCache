cmake_minimum_required(VERSION 3.20)
project(MiniCache VERSION 1.5.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Set binary output directory to Workspace/MiniCache/bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

include_directories(include)

# Build minicache static library
add_library(minicache_lib
    src/cache_store.cpp
    src/command_factory.cpp
    src/aof_engine.cpp
    src/resp_parser.cpp
    src/server.cpp
)

# Server Executable -> bin/minicache_server
add_executable(minicache_server src/main.cpp)
target_link_libraries(minicache_server PRIVATE minicache_lib)

# Test executables -> bin/test_*
add_executable(test_cache tests/test_cache.cpp)
target_link_libraries(test_cache PRIVATE minicache_lib)

add_executable(test_command tests/test_command.cpp)
target_link_libraries(test_command PRIVATE minicache_lib)

add_executable(test_object_pool tests/test_object_pool.cpp)
target_link_libraries(test_object_pool PRIVATE minicache_lib)

add_executable(test_aof tests/test_aof.cpp)
target_link_libraries(test_aof PRIVATE minicache_lib)

add_executable(test_server tests/test_server.cpp)
target_link_libraries(test_server PRIVATE minicache_lib)