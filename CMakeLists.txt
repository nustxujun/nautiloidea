cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_SYSTEM_VERSION "10.0")

project(nautiloidea)

add_definitions(-D_CONSOLE)
add_definitions(-DNO_UE4)
add_definitions(-DWIN32_LEAN_AND_MEAN)

#std::wstring_convert is deprecated in c++17, but there is no easy way to replace it. 
#display warnings and remain using it
add_definitions(-D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
set(IsOnWin7 FALSE CACHE BOOL "if operation system is win7" )


include_directories(${CMAKE_SOURCE_DIR}/dxsdk/Include)

file(GLOB headers *.h)
source_group(src FILES ${headers})

AUX_SOURCE_DIRECTORY(. localsources)
source_group(src FILES ${localsources})


set(ENGINE "engine")
include_directories(${ENGINE})
add_subdirectory(${ENGINE})


if (${IsOnWin7} STREQUAL ON)
    add_definitions(-DD3D12ON7)
endif()

add_executable(nautiloidea ${localsources} ${enginesources} ${headers} )

add_dependencies(nautiloidea hermitcrab)
target_link_libraries(nautiloidea debug hermitcrab)
target_link_libraries(nautiloidea optimized hermitcrab)