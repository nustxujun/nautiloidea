cmake_minimum_required(VERSION 3.15.0)


add_definitions(-DWIN32_LEAN_AND_MEAN)

#std::wstring_convert is deprecated in c++17, but there is no easy way to replace it. 
#display warnings and remain using it
add_definitions(-D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest /EHsc")


# assimp settings
set(ASSIMP_BUILD_ASSIMP_TOOLS FALSE CACHE BOOL "")
set(ASSIMP_BUILD_TESTS FALSE CACHE BOOL "")
set(ASSIMP_INSTALL_PDB FALSE CACHE BOOL "")
set(BUILD_XAUDIO_WIN10 FALSE CACHE BOOL "")
set(HUNTER_STATUS_PRINT FALSE CACHE BOOL "")
set(HUNTER_TLS_VERIFY FALSE CACHE BOOL "")
set(INJECT_DEBUG_POSTFIX FALSE CACHE BOOL "")
set(INJECT_DEBUG_POSTFIX FALSE CACHE BOOL "")
set(BUILD_SHARED_LIBS FALSE CACHE BOOL "")

# engine settings
set(ASIO_DIRECTORY ${CMAKE_SOURCE_DIR}/external/asio/asio/include CACHE STRING "asio include directory")

include_directories(${CMAKE_SOURCE_DIR}/external/asio/asio/include)


project(nautiloidea)

add_subdirectory(engine)
add_subdirectory(plugins)
#add_subdirectory(ExampleExplorer)
#add_subdirectory(RemoteRender)
add_subdirectory(Editor)

add_subdirectory(assimp)
add_subdirectory(DirectXTK12)

set_property(TARGET Editor PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")



