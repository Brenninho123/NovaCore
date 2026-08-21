cmake_minimum_required(VERSION 3.16)
project(NovaCore CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)

add_executable(NovaCore project.cpp)

target_include_directories(NovaCore PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(NovaCore PRIVATE ${SDL2_LIBRARIES})

if(WIN32)
    target_link_libraries(NovaCore PRIVATE mingw32 SDL2main)
endif()
