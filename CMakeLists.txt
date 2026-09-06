cmake_minimum_required(VERSION 3.20)
project(MiniCache VERSION 1.0.0 LANGUAGES CXX)

# Force C++20 Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Include directories
include_directories(include)

# Library target
add_library(minicache_lib
    src/cache_store.cpp
)

# Test executable
add_executable(test_cache
    tests/test_cache.cpp
)

target_link_libraries(test_cache PRIVATE minicache_lib)