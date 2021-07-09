project(Editor)

include_directories(${CMAKE_BINARY_DIR}/assimp/include)
include_directories(../assimp/include)
include_directories(../engine)
include_directories(../plugins)
include_directories(../DirectXTK12/Inc)
include_directories(../DirectXMath/Inc)

add_definitions(-DNO_UE4)

AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR} EXAMPLE_SOURCE)
add_executable(Editor ${EXAMPLE_SOURCE} )

#dependences
set(projects hermitcrab plugins assimp DirectXTK12)

add_dependencies(Editor ${projects})

target_link_libraries(Editor debug ${projects})
target_link_libraries(Editor optimized ${projects})

set_property(TARGET Editor PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
