cmake_minimum_required(VERSION 3.20)
project(minidb VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Library ────────────────────────────────────────────────────────────────────
add_library(minidb
    src/pager.cpp
    src/database.cpp
)

target_include_directories(minidb PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_options(minidb PRIVATE
    -Wall -Wextra -Wpedantic
)

# ── Tests ──────────────────────────────────────────────────────────────────────
enable_testing()

add_executable(test_entry    tests/main.cpp tests/test_entry.cpp)
add_executable(test_pager    tests/main.cpp tests/test_pager.cpp)
add_executable(test_buffer   tests/main.cpp tests/test_buffer.cpp)
add_executable(test_database tests/main.cpp tests/test_database.cpp)

foreach(target test_entry test_pager test_buffer test_database)
    target_include_directories(${target} PRIVATE include)
    target_link_libraries(${target} PRIVATE minidb)
    target_compile_options(${target} PRIVATE -Wall -Wextra)
    add_test(NAME ${target} COMMAND ${target})
endforeach()