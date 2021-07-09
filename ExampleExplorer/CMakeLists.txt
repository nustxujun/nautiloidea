project(ExampleExplorer)

include_directories(${CMAKE_BINARY_DIR}/assimp/include)
include_directories(../assimp/include)
include_directories(../engine)
include_directories(../plugins)
include_directories(../RemoteRender)

add_definitions(-DNO_UE4)

AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR} EXAMPLE_SOURCE)
add_executable(ExampleExplorer ${EXAMPLE_SOURCE} )

#dependences
set(projects hermitcrab plugins RemoteRender assimp)

add_dependencies(ExampleExplorer ${projects})

target_link_libraries(ExampleExplorer debug ${projects})
target_link_libraries(ExampleExplorer optimized ${projects})

#nuget deps
configure_file(packages.config packages.config COPYONLY)
configure_file(deps.props deps.props COPYONLY)
set_property( TARGET ExampleExplorer APPEND PROPERTY VS_USER_PROPS deps.props )  

