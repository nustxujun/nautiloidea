project(Editor)

add_definitions(-DNO_UE4)

include_directories(${CMAKE_BINARY_DIR}/assimp/include)
include_directories(../assimp/include)
include_directories(../engine)
include_directories(../plugins)
include_directories(../DirectXTK12/Inc)
include_directories(../DirectXMath/Inc)
include_directories(../external)
include_directories(../external/lua)
link_directories(../external/lua)
include_directories(../external/sol2/include)


#dependences
set(projects hermitcrab plugins assimp DirectXTK12)
set(externals lua)

# scan sources
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR} EXAMPLE_SOURCE)
# make execution
add_executable(Editor ${EXAMPLE_SOURCE} )

# add library
add_dependencies(Editor ${projects})

target_link_libraries(Editor debug ${projects} ${externals})
target_link_libraries(Editor optimized ${projects} ${externals})

# set working directory
set_property(TARGET Editor PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
