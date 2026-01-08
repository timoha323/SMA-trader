cmake_minimum_required(VERSION 3.16)
project(SMA_trader_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(server
    server.cpp
    ../file_processing/file_proccessing.cpp
    ../tools/SMA/price_buffer.cpp
)

target_include_directories(server PRIVATE
    ../tools
    ../file_processing
)

find_package(Threads REQUIRED)
target_link_libraries(server PRIVATE Threads::Threads)

if (WIN32)
    target_link_libraries(server PRIVATE ws2_32)
endif()
