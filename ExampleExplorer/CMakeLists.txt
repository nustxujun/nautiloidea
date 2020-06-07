project(ExampleExplorer)

include_directories(../engine)
# add_subdirectory(../engine external/engine)

include_directories(../Examples)

AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR} EXAMPLE_SOURCE)
add_executable(ExampleExplorer ${EXAMPLE_SOURCE} )

set(projects hermitcrab)

add_dependencies(ExampleExplorer ${projects})

target_link_libraries(ExampleExplorer debug ${projects})
target_link_libraries(ExampleExplorer optimized ${projects})



configure_file(packages.config packages.config COPYONLY)

set(DEPS_PROPS deps.props)
configure_file(${DEPS_PROPS} ${DEPS_PROPS} COPYONLY)

set_property( TARGET ExampleExplorer APPEND PROPERTY VS_USER_PROPS ${DEPS_PROPS} )  