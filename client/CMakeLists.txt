cmake_minimum_required(VERSION 3.16)
project(SMA_trader_client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(client
    client.cpp
    ../tools/price_generator/price_generator.cpp
)

target_include_directories(client PRIVATE
    ../tools
)

find_package(Threads REQUIRED)
target_link_libraries(client PRIVATE Threads::Threads)

if (WIN32)
    target_link_libraries(client PRIVATE ws2_32)
endif()
