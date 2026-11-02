cmake_minimum_required(VERSION 3.16)
project(SwitchAudiobook VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT DEFINED ENV{DEVKITPRO})
    message(FATAL_ERROR "DEVKITPRO environment variable not set. Please source /opt/devkitpro/devkita64.sh")
endif()

set(CMAKE_TOOLCHAIN_FILE "$ENV{DEVKITPRO}/cmake/Switch.cmake" CACHE PATH "Switch toolchain file")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -O2 -fPIE")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Map=${PROJECT_NAME}.map")

find_package(libnx REQUIRED)
find_package(ffmpeg QUIET)
find_package(mpv REQUIRED)
find_package(borealis REQUIRED)
find_package(curl REQUIRED)
find_package(mbedtls REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(zlib REQUIRED)
find_package(bzip2 REQUIRED)
find_package(freetype REQUIRED)
find_package(nlohmann_json REQUIRED)

if(NOT ffmpeg_FOUND)
    message(WARNING "ffmpeg not found, video decoding features disabled")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SOURCES
    src/main.cpp
    src/core/Application.cpp
    src/core/Config.cpp
    src/bilibili/BiliBiliClient.cpp
    src/network/HttpClient.cpp
    src/player/AudioPlayer.cpp
    src/ui/MainUI.cpp
    src/ui/SearchView.cpp
    src/ui/PlayerView.cpp
    src/ui/SettingsView.cpp
    src/utils/Logger.cpp
)

file(GLOB_RECURSE FONTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/*.ttf")

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE
    nx
    borealis
    curl
    mbedtls
    mbedx509
    mbedcrypto
    OpenSSL::Crypto
    zlib
    bzip2
    freetype
    nlohmann_json
    ${DEVKITPRO_LIBRARIES}
)

if(ffmpeg_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE ffmpeg)
endif()
if(mpv_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE mpv)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
    SWITCH_AUDIOBOOK_VERSION="${PROJECT_VERSION}"
    DEFAULT_FONT_PATH="romfs:/fonts/NotoSansCJKsc-Regular.ttf"
)

add_custom_target(romfs ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/romfs/fonts
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/NotoSansCJKsc-Regular.ttf
        ${CMAKE_CURRENT_BINARY_DIR}/romfs/fonts/
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/romfs
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/ca-bundle.crt
        ${CMAKE_CURRENT_BINARY_DIR}/romfs/
    DEPENDS ${FONTS}
    COMMENT "Copying assets to romfs"
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${DEVKITPRO}/tools/bin/nrotool
        -o ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.nro
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.elf
        --romfsdir=${CMAKE_CURRENT_BINARY_DIR}/romfs
        --icon=${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.jpg
        --nacp=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.nacp
    COMMENT "Building NRO"
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/meta/nacp.json.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.nacp
    @ONLY)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION /switch/${PROJECT_NAME}
)

enable_testing()
add_test(NAME AudiobookTest COMMAND ${PROJECT_NAME} --test)