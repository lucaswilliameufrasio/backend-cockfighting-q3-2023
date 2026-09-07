cmake_minimum_required(VERSION 3.15)
project(backend-cockfighting-api)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Try to find system uuid first (Drogon needs it)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(UUID libuuid)
endif()

if(NOT UUID_FOUND)
    find_library(UUID_LIBRARIES uuid)
    find_path(UUID_INCLUDE_DIRS uuid/uuid.h)
endif()

find_package(Drogon REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(mimalloc REQUIRED)
find_package(simdjson REQUIRED)

add_executable(${PROJECT_NAME} src/main.cc)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(${PROJECT_NAME} PRIVATE -O3 -march=native -flto)
    target_link_options(${PROJECT_NAME} PRIVATE -flto)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    ZLIB::ZLIB
    Drogon::Drogon
    PostgreSQL::PostgreSQL
    mimalloc-static
    simdjson::simdjson
)

if(UUID_LIBRARIES)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${UUID_LIBRARIES})
    target_include_directories(${PROJECT_NAME} PRIVATE ${UUID_INCLUDE_DIRS})
endif()

# --- Testes unitários (GoogleTest via Conan [test_requires]) ---
find_package(GTest CONFIG REQUIRED)
enable_testing()
add_executable(unit-tests src/tests/unit_tests.cc)
target_link_libraries(unit-tests PRIVATE
    GTest::gtest_main
    Drogon::Drogon
    ZLIB::ZLIB
    mimalloc-static
    simdjson::simdjson
)
if(UUID_LIBRARIES)
    target_link_libraries(unit-tests PRIVATE ${UUID_LIBRARIES})
    target_include_directories(unit-tests PRIVATE ${UUID_INCLUDE_DIRS})
endif()
include(GoogleTest)
gtest_discover_tests(unit-tests)
