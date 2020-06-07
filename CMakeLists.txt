cmake_minimum_required(VERSION 3.15.0)


add_definitions(-DWIN32_LEAN_AND_MEAN)

#std::wstring_convert is deprecated in c++17, but there is no easy way to replace it. 
#display warnings and remain using it
add_definitions(-D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")



project(nautiloidea)

add_subdirectory(engine)
add_subdirectory(ExampleExplorer)
set_property(TARGET ExampleExplorer PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")



