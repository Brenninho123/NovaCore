cmake_minimum_required(VERSION 3.16)

project(NovaCore
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(NOVACORE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(NOVACORE_ENABLE_LTO "Enable link-time optimization on Release builds" ON)

find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_mixer REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glad CONFIG REQUIRED)

set(NOVACORE_CORE_SOURCES
    source/Main.cpp
)

set(NOVACORE_CORE_HEADERS
)

set(NOVACORE_ASSETS_SOURCES
    source/novacore/Assets.cpp
)

set(NOVACORE_ASSETS_HEADERS
    source/novacore/Assets.h
)

set(NOVACORE_BACKEND_SOURCES
    source/novacore/backend/Controls.cpp
)

set(NOVACORE_BACKEND_HEADERS
    source/novacore/backend/Controls.h
)

set(NOVACORE_STATES_SOURCES
    source/novacore/states/MenuState.cpp
)

set(NOVACORE_STATES_HEADERS
    source/novacore/states/State.h
    source/novacore/states/MenuState.h
)

set(NOVACORE_SHADERS_SOURCES
    source/novacore/shaders/ShaderManager.cpp
)

set(NOVACORE_SHADERS_HEADERS
    source/novacore/shaders/ShaderManager.h
)

set(NOVACORE_ALL_SOURCES
    ${NOVACORE_CORE_SOURCES}
    ${NOVACORE_ASSETS_SOURCES}
    ${NOVACORE_BACKEND_SOURCES}
    ${NOVACORE_STATES_SOURCES}
    ${NOVACORE_SHADERS_SOURCES}
)

set(NOVACORE_ALL_HEADERS
    ${NOVACORE_CORE_HEADERS}
    ${NOVACORE_ASSETS_HEADERS}
    ${NOVACORE_BACKEND_HEADERS}
    ${NOVACORE_STATES_HEADERS}
    ${NOVACORE_SHADERS_HEADERS}
)

set(NOVACORE_ICON_PNG "${CMAKE_SOURCE_DIR}/arts/icon.png")
set(NOVACORE_ICON_ICO "${CMAKE_BINARY_DIR}/generated/icon.ico")
set(NOVACORE_RC_FILE "")

if(WIN32 AND EXISTS "${NOVACORE_ICON_PNG}")
    find_program(NOVACORE_MAGICK_EXE NAMES magick convert)

    if(NOVACORE_MAGICK_EXE)
        file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated")

        add_custom_command(
            OUTPUT "${NOVACORE_ICON_ICO}"
            COMMAND ${NOVACORE_MAGICK_EXE} "${NOVACORE_ICON_PNG}" -define icon:auto-resize=256,128,64,48,32,16 "${NOVACORE_ICON_ICO}"
            DEPENDS "${NOVACORE_ICON_PNG}"
            COMMENT "Generating icon.ico from arts/icon.png"
            VERBATIM
        )

        add_custom_target(NovaCoreIcon DEPENDS "${NOVACORE_ICON_ICO}")

        set(NOVACORE_RC_IN "${CMAKE_BINARY_DIR}/generated/resource.rc.in")
        file(WRITE "${NOVACORE_RC_IN}" "IDI_ICON1 ICON \"${NOVACORE_ICON_ICO}\"\n")

        set(NOVACORE_RC_FILE "${CMAKE_BINARY_DIR}/generated/resource.rc")
        configure_file("${NOVACORE_RC_IN}" "${NOVACORE_RC_FILE}" COPYONLY)
    else()
        message(WARNING "arts/icon.png found but ImageMagick (magick/convert) is not installed - skipping icon embedding")
    endif()
endif()

add_executable(NovaCore ${NOVACORE_ALL_SOURCES} ${NOVACORE_ALL_HEADERS} ${NOVACORE_RC_FILE})

if(TARGET NovaCoreIcon)
    add_dependencies(NovaCore NovaCoreIcon)
endif()

source_group("Core" FILES ${NOVACORE_CORE_SOURCES} ${NOVACORE_CORE_HEADERS})
source_group("Assets" FILES ${NOVACORE_ASSETS_SOURCES} ${NOVACORE_ASSETS_HEADERS})
source_group("Backend" FILES ${NOVACORE_BACKEND_SOURCES} ${NOVACORE_BACKEND_HEADERS})
source_group("States" FILES ${NOVACORE_STATES_SOURCES} ${NOVACORE_STATES_HEADERS})
source_group("Shaders" FILES ${NOVACORE_SHADERS_SOURCES} ${NOVACORE_SHADERS_HEADERS})
if(NOVACORE_RC_FILE)
    source_group("Resources" FILES ${NOVACORE_RC_FILE})
endif()

target_include_directories(NovaCore PRIVATE
    source
)

target_link_libraries(NovaCore PRIVATE
    $<IF:$<TARGET_EXISTS:SDL2::SDL2main>,SDL2::SDL2main,>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    $<IF:$<TARGET_EXISTS:SDL2_image::SDL2_image>,SDL2_image::SDL2_image,SDL2_image::SDL2_image-static>
    $<IF:$<TARGET_EXISTS:SDL2_mixer::SDL2_mixer>,SDL2_mixer::SDL2_mixer,SDL2_mixer::SDL2_mixer-static>
    glad::glad
    OpenGL::GL
)

if(MINGW)
    target_link_libraries(NovaCore PRIVATE mingw32)
endif()

target_compile_definitions(NovaCore PRIVATE
    $<$<CONFIG:Debug>:NOVACORE_DEBUG>
    $<$<CONFIG:Release>:NOVACORE_RELEASE>
)

if(MSVC)
    target_compile_options(NovaCore PRIVATE /W4 /permissive-)
    if(NOVACORE_WARNINGS_AS_ERRORS)
        target_compile_options(NovaCore PRIVATE /WX)
    endif()
else()
    target_compile_options(NovaCore PRIVATE -Wall -Wextra -Wpedantic)
    if(NOVACORE_WARNINGS_AS_ERRORS)
        target_compile_options(NovaCore PRIVATE -Werror)
    endif()
endif()

include(CheckIPOSupported)
check_ipo_supported(RESULT NOVACORE_IPO_SUPPORTED OUTPUT NOVACORE_IPO_ERROR)

if(NOVACORE_ENABLE_LTO AND NOVACORE_IPO_SUPPORTED AND CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(NovaCore PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

if(MSVC)
    set_target_properties(NovaCore PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:NovaCore>"
    )
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
    add_custom_command(TARGET NovaCore POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:NovaCore>/assets"
    )
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/shaders")
    add_custom_command(TARGET NovaCore POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/shaders"
            "$<TARGET_FILE_DIR:NovaCore>/shaders"
    )
endif()

install(TARGETS NovaCore RUNTIME DESTINATION .)

if(WIN32)
    install(FILES $<TARGET_RUNTIME_DLLS:NovaCore> DESTINATION . OPTIONAL)
endif()
