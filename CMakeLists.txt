cmake_minimum_required(VERSION 3.20)

project(TinyTroopers LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Option to build only the dedicated server without client / raylib dependencies
option(BUILD_SERVER_ONLY "Build only the dedicated server (headless)" OFF)

# Default to Release build if no build type is specified (for single-config generators)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

# Compiler optimizations for Release builds
if(MSVC)
    add_compile_options("$<$<CONFIG:Release>:/O2;/Oi;/Ot;/Oy;/Gy;/GF;/DNDEBUG>")
else()
    add_compile_options("$<$<CONFIG:Release>:-O3;-DNDEBUG>")
endif()

# Common network and protocol sources
set(COMMON_SOURCES
    src/common/Vec2.cpp
    src/common/UdpSocket.cpp
    src/common/Protocol.cpp
)

# Dedicated Server Target
add_executable(TinyTroopersServer
    src/server/main.cpp
    src/server/GameServer.cpp
    src/server/Room.cpp
    src/server/HttpServer.cpp
    ${COMMON_SOURCES}
)

target_include_directories(TinyTroopersServer PRIVATE src)

if(WIN32)
    target_link_libraries(TinyTroopersServer PRIVATE ws2_32)
    if(MINGW)
        target_link_options(TinyTroopersServer PRIVATE -static-libgcc -static-libstdc++ -static)
    endif()
endif()

# Client and Tool Targets (skipped if BUILD_SERVER_ONLY=ON)
if(NOT BUILD_SERVER_ONLY)
    find_package(raylib CONFIG QUIET)
    if(NOT raylib_FOUND AND EXISTS "C:/raylib/w64devkit/include/raylib.h")
        add_library(raylib STATIC IMPORTED GLOBAL)
        set_target_properties(raylib PROPERTIES
            IMPORTED_LOCATION "C:/raylib/w64devkit/lib/libraylib.a"
            INTERFACE_INCLUDE_DIRECTORIES "C:/raylib/w64devkit/include")
        set(raylib_FOUND TRUE)
    endif()
    if(NOT raylib_FOUND)
        message(FATAL_ERROR "raylib was not found. Install official raylib to C:/raylib or set raylib_DIR/CMAKE_PREFIX_PATH, or run cmake -DBUILD_SERVER_ONLY=ON.")
    endif()

    add_executable(TinyTroopersClient
        src/client/main.cpp
        src/client/GameClient.cpp
        src/client/NetworkClient.cpp
        src/client/AssetManager.cpp
        src/client/AudioManager.cpp
        src/client/MainMenuScreen.cpp
        src/client/RoomCreationScreen.cpp
        src/client/RoomJoiningScreen.cpp
        src/client/GameplayScreen.cpp
        src/client/ScreenManager.cpp
        ${COMMON_SOURCES}
    )

    add_executable(RaylibSmokeTest
        src/tools/RaylibSmokeTest.cpp
    )

    target_include_directories(TinyTroopersClient PRIVATE src)
    target_include_directories(RaylibSmokeTest PRIVATE src)

    target_link_libraries(TinyTroopersClient PRIVATE raylib)
    target_link_libraries(RaylibSmokeTest PRIVATE raylib)

    if(WIN32)
        target_link_libraries(TinyTroopersClient PRIVATE ws2_32 winmm gdi32 opengl32)
        target_link_libraries(RaylibSmokeTest PRIVATE winmm gdi32 opengl32)
        if(MINGW)
            target_link_options(TinyTroopersClient PRIVATE -static-libgcc -static-libstdc++ -static -Wl,--allow-multiple-definition)
            target_link_options(RaylibSmokeTest PRIVATE -static-libgcc -static-libstdc++ -static -Wl,--allow-multiple-definition)
        endif()
    endif()

    add_custom_command(TARGET TinyTroopersClient POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/assets
                $<TARGET_FILE_DIR:TinyTroopersClient>/assets)
endif()
