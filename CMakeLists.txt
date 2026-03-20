cmake_minimum_required(VERSION 3.15)
project(backend-cockfighting-api)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
