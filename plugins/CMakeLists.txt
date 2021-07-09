cmake_minimum_required(VERSION 3.0.0)

project(plugins)

include_directories(../engine)
include_directories(${CMAKE_BINARY_DIR}/assimp/include)
include_directories(../assimp/include)


file(GLOB PLIGINS_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
source_group(src FILES ${PLIGINS_HEADERS})

AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR} PLUGINS_SOURCES)
source_group(src FILES ${PLUGINS_SOURCES})

add_library(plugins STATIC ${PLUGINS_SOURCES} ${PLIGINS_HEADERS})

set(PLUGINS_DEPS hermitcrab)

add_dependencies(plugins ${PLUGINS_DEPS})

target_link_libraries(plugins debug ${PLUGINS_DEPS})
target_link_libraries(plugins optimized ${PLUGINS_DEPS})


#nuget deps
configure_file(packages.config packages.config COPYONLY)
configure_file(deps.props deps.props COPYONLY)
set_property( TARGET plugins APPEND PROPERTY VS_USER_PROPS deps.props )  