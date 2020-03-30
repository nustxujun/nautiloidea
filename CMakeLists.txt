cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_SYSTEM_VERSION "10.0")

project(nautiloidea)

set(IsOnWin7 FALSE CACHE BOOL "if operation system is win7" )


include_directories(${CMAKE_SOURCE_DIR}/Engine)
include_directories(${CMAKE_SOURCE_DIR}/dxsdk/Include)

function(collectSourceFiles container subpath)
if (${subpath} STREQUAL ${CMAKE_HOME_DIRECTORY})
    file(GLOB files  ${CMAKE_HOME_DIRECTORY}/*.cpp ${CMAKE_HOME_DIRECTORY}/*.h)
else()
    file(GLOB_RECURSE files  ${subpath}/*.cpp ${subpath}/*.h)
endif()
    foreach(v ${files})
        message(STATUS "collect file: " ${v})
        string(REPLACE ${CMAKE_HOME_DIRECTORY}/ "" RELPATH ${v})
        get_filename_component(FILTER ${RELPATH} DIRECTORY)
        if (FILTER)
            string(REPLACE / \\ FILTER ${FILTER})
            source_group(${FILTER} FILES ${RELPATH})
        else()
            source_group("src" FILES ${RELPATH})
        endif()
    endforeach()
    set(${container} ${${container}} ${files} PARENT_SCOPE)
endfunction()


collectSourceFiles(sourcefiles ${CMAKE_HOME_DIRECTORY})
collectSourceFiles(sourcefiles Engine)

add_definitions(-D_CONSOLE)
add_definitions(-DNO_UE4)
add_definitions(-DWIN32_LEAN_AND_MEAN)

if (${IsOnWin7} STREQUAL ON)
    add_definitions(-DD3D12ON7)
endif()

add_executable(nautiloidea ${sourcefiles})

