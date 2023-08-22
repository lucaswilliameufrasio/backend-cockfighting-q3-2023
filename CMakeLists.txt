cmake_minimum_required(VERSION 3.15)
project(backend-cockfighting-api)

find_package(Drogon REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(ZLIB REQUIRED)

add_executable(${PROJECT_NAME} src/main.cc)
target_link_libraries(${PROJECT_NAME} ZLIB::ZLIB)
target_link_libraries(${PROJECT_NAME} Drogon::Drogon)
target_link_libraries(${PROJECT_NAME} PostgreSQL::PostgreSQL)
