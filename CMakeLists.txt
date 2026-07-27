cmake_minimum_required(VERSION 3.20)

set(CMAKE_TOOLCHAIN_FILE "/opt/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "Vcpkg toolchain file")

if(NOT VCPKG_TARGET_TRIPLET)
    if(WIN32)
        set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "")
    else()
        set(VCPKG_TARGET_TRIPLET "x64-linux" CACHE STRING "")
    endif()
endif()

project(serverseeker CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_PROFILING "-O2 -g -fno-omit-frame-pointer")
set(CMAKE_C_FLAGS_PROFILING   "-O2 -g -fno-omit-frame-pointer")

set(BUILD_SHARED_LIBS OFF)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    add_compile_options(-static-libgcc -static-libstdc++)
    add_compile_options(-fno-omit-frame-pointer)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
endif()


file(GLOB_RECURSE SERVERSEEKER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.cc"
    "${CMAKE_SOURCE_DIR}/src/*.hpp"
    "${CMAKE_SOURCE_DIR}/src/*.h"
)

add_executable(serverseeker
    main.cpp
    ${SERVERSEEKER_SOURCES}
)

target_include_directories(serverseeker PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

find_package(spdlog CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)
find_package(Boost REQUIRED)
find_package(reproc++ CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)
find_package(args CONFIG REQUIRED)

target_link_libraries(serverseeker PRIVATE
    spdlog::spdlog
    libpqxx::pqxx
    Boost::boost
    reproc++
    nlohmann_json::nlohmann_json
    httplib::httplib
    taywee::args
)
