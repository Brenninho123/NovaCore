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
option(NOVACORE_ENABLE_SANITIZERS "Enable address/UB sanitizers on Debug builds (desktop only)" OFF)
option(NOVACORE_ENABLE_DISCORD "Enable Discord OAuth login module (desktop only)" ON)

set(NOVACORE_IS_MOBILE OFF)
if(ANDROID OR IOS OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(NOVACORE_IS_MOBILE ON)
endif()

if(NOVACORE_IS_MOBILE)
    set(NOVACORE_ENABLE_DISCORD OFF)
endif()

find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_mixer REQUIRED)

if(NOT ANDROID)
    find_package(OpenGL REQUIRED)
    find_package(glad CONFIG REQUIRED)
endif()

if(NOVACORE_ENABLE_DISCORD)
    find_package(CURL REQUIRED)
    find_package(OpenSSL REQUIRED)
    find_package(nlohmann_json CONFIG REQUIRED)
endif()

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

set(NOVACORE_MOBILE_SOURCES
    source/novacore/mobile/ScreenUtil.cpp
)

set(NOVACORE_MOBILE_HEADERS
    source/novacore/mobile/ScreenUtil.h
)

set(NOVACORE_UI_SOURCES
    source/novacore/ui/Guard.cpp
)

set(NOVACORE_UI_HEADERS
    source/novacore/ui/Guard.h
)

set(NOVACORE_DISCORD_SOURCES
    source/novacore/api/discord/DiscordLogin.cpp
)

set(NOVACORE_DISCORD_HEADERS
    source/novacore/api/discord/DiscordLogin.h
)

set(NOVACORE_ALL_SOURCES
    ${NOVACORE_CORE_SOURCES}
    ${NOVACORE_ASSETS_SOURCES}
    ${NOVACORE_BACKEND_SOURCES}
    ${NOVACORE_STATES_SOURCES}
    ${NOVACORE_SHADERS_SOURCES}
    ${NOVACORE_MOBILE_SOURCES}
    ${NOVACORE_UI_SOURCES}
)

set(NOVACORE_ALL_HEADERS
    ${NOVACORE_CORE_HEADERS}
    ${NOVACORE_ASSETS_HEADERS}
    ${NOVACORE_BACKEND_HEADERS}
    ${NOVACORE_STATES_HEADERS}
    ${NOVACORE_SHADERS_HEADERS}
    ${NOVACORE_MOBILE_HEADERS}
    ${NOVACORE_UI_HEADERS}
)

if(NOVACORE_ENABLE_DISCORD)
    list(APPEND NOVACORE_ALL_SOURCES ${NOVACORE_DISCORD_SOURCES})
    list(APPEND NOVACORE_ALL_HEADERS ${NOVACORE_DISCORD_HEADERS})
endif()

set(NOVACORE_ICON_PNG "${CMAKE_SOURCE_DIR}/arts/icon.png")
set(NOVACORE_ICON_ICO "${CMAKE_BINARY_DIR}/generated/icon.ico")
set(NOVACORE_RC_FILE "")

if(WIN32 AND NOT ANDROID AND EXISTS "${NOVACORE_ICON_PNG}")
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

if(ANDROID)
    add_library(NovaCore SHARED ${NOVACORE_ALL_SOURCES} ${NOVACORE_ALL_HEADERS})
    set_target_properties(NovaCore PROPERTIES
        LIBRARY_OUTPUT_NAME "main"
        POSITION_INDEPENDENT_CODE ON
    )
elseif(NOVACORE_IS_MOBILE)
    add_executable(NovaCore MACOSX_BUNDLE ${NOVACORE_ALL_SOURCES} ${NOVACORE_ALL_HEADERS})
else()
    add_executable(NovaCore ${NOVACORE_ALL_SOURCES} ${NOVACORE_ALL_HEADERS} ${NOVACORE_RC_FILE})
endif()

if(TARGET NovaCoreIcon)
    add_dependencies(NovaCore NovaCoreIcon)
endif()

source_group("Core" FILES ${NOVACORE_CORE_SOURCES} ${NOVACORE_CORE_HEADERS})
source_group("Assets" FILES ${NOVACORE_ASSETS_SOURCES} ${NOVACORE_ASSETS_HEADERS})
source_group("Backend" FILES ${NOVACORE_BACKEND_SOURCES} ${NOVACORE_BACKEND_HEADERS})
source_group("States" FILES ${NOVACORE_STATES_SOURCES} ${NOVACORE_STATES_HEADERS})
source_group("Shaders" FILES ${NOVACORE_SHADERS_SOURCES} ${NOVACORE_SHADERS_HEADERS})
source_group("Mobile" FILES ${NOVACORE_MOBILE_SOURCES} ${NOVACORE_MOBILE_HEADERS})
source_group("UI" FILES ${NOVACORE_UI_SOURCES} ${NOVACORE_UI_HEADERS})
if(NOVACORE_ENABLE_DISCORD)
    source_group("API\\Discord" FILES ${NOVACORE_DISCORD_SOURCES} ${NOVACORE_DISCORD_HEADERS})
endif()
if(NOVACORE_RC_FILE)
    source_group("Resources" FILES ${NOVACORE_RC_FILE})
endif()

target_include_directories(NovaCore PRIVATE
    source
)

if(ANDROID)
    target_link_libraries(NovaCore PRIVATE
        $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
        $<IF:$<TARGET_EXISTS:SDL2_image::SDL2_image>,SDL2_image::SDL2_image,SDL2_image::SDL2_image-static>
        $<IF:$<TARGET_EXISTS:SDL2_mixer::SDL2_mixer>,SDL2_mixer::SDL2_mixer,SDL2_mixer::SDL2_mixer-static>
        GLESv2
        log
        android
    )
else()
    target_link_libraries(NovaCore PRIVATE
        $<IF:$<TARGET_EXISTS:SDL2::SDL2main>,SDL2::SDL2main,>
        $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
        $<IF:$<TARGET_EXISTS:SDL2_image::SDL2_image>,SDL2_image::SDL2_image,SDL2_image::SDL2_image-static>
        $<IF:$<TARGET_EXISTS:SDL2_mixer::SDL2_mixer>,SDL2_mixer::SDL2_mixer,SDL2_mixer::SDL2_mixer-static>
        glad::glad
        OpenGL::GL
    )
endif()

if(NOVACORE_ENABLE_DISCORD)
    target_link_libraries(NovaCore PRIVATE
        CURL::libcurl
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
    )

    target_compile_definitions(NovaCore PRIVATE NOVACORE_DISCORD_ENABLED)

    if(WIN32)
        target_link_libraries(NovaCore PRIVATE ws2_32)
    else()
        find_package(Threads REQUIRED)
        target_link_libraries(NovaCore PRIVATE Threads::Threads)
    endif()
endif()

if(MINGW)
    target_link_libraries(NovaCore PRIVATE mingw32)
endif()

target_compile_definitions(NovaCore PRIVATE
    $<$<CONFIG:Debug>:NOVACORE_DEBUG>
    $<$<CONFIG:Release>:NOVACORE_RELEASE>
    $<$<BOOL:${ANDROID}>:NOVACORE_ANDROID>
    $<$<BOOL:${NOVACORE_IS_MOBILE}>:NOVACORE_MOBILE>
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

if(NOVACORE_ENABLE_SANITIZERS AND NOT ANDROID AND NOT NOVACORE_IS_MOBILE AND NOT MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(NovaCore PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(NovaCore PRIVATE -fsanitize=address,undefined)
endif()

if(ANDROID)
    target_compile_options(NovaCore PRIVATE -ffunction-sections -fdata-sections)
    target_link_options(NovaCore PRIVATE -Wl,--gc-sections)

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_link_options(NovaCore PRIVATE -s)
    endif()
endif()

if(NOT ANDROID)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT NOVACORE_IPO_SUPPORTED OUTPUT NOVACORE_IPO_ERROR)

    if(NOVACORE_ENABLE_LTO AND NOVACORE_IPO_SUPPORTED AND CMAKE_BUILD_TYPE STREQUAL "Release")
        set_target_properties(NovaCore PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endif()

if(MSVC)
    set_target_properties(NovaCore PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:NovaCore>"
    )
endif()

if(NOT ANDROID AND NOT NOVACORE_IS_MOBILE)
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
endif()

if(ANDROID AND EXISTS "${CMAKE_SOURCE_DIR}/assets")
    file(COPY "${CMAKE_SOURCE_DIR}/assets" DESTINATION "${CMAKE_SOURCE_DIR}/android-project/app/src/main/assets/novacore")
endif()

install(TARGETS NovaCore
    RUNTIME DESTINATION .
    LIBRARY DESTINATION .
)

if(WIN32 AND NOT ANDROID)
    install(FILES $<TARGET_RUNTIME_DLLS:NovaCore> DESTINATION . OPTIONAL)
endif()

message(STATUS "NovaCore target: ${CMAKE_SYSTEM_NAME} (mobile=${NOVACORE_IS_MOBILE}, android=${ANDROID}, discord=${NOVACORE_ENABLE_DISCORD})")
