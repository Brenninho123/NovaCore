cmake_minimum_required(VERSION 3.16)
project(NovaCore CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)

add_executable(NovaCore
    source/Main.cpp
    source/novacore/Assets.cpp
    source/novacore/backend/Controls.cpp
    source/novacore/states/MenuState.cpp
)

target_include_directories(NovaCore PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
    source
)

target_link_libraries(NovaCore PRIVATE
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
)

if(MINGW)
    target_link_libraries(NovaCore PRIVATE mingw32 SDL2main)
endif()
