project(armory)
cmake_minimum_required(VERSION 2.6)

include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/common.cmake)

message(STATUS "Building armory importer")

# Qt5 stuff
# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

add_definitions(-DQT_PLUGIN)

set(src ArmoryImporter.cpp)

set(headers ArmoryImporter.h)

source_group("Header Files" FILES ${headers})

set(useful_files armoryimporter.json)
set_source_files_properties(${useful_files} PROPERTIES HEADER_FILE_ONLY TRUE)

use_core()
use_wow()

set(NAME armory)
add_library(${NAME} SHARED ${src} ${headers} ${useful_files})
set_property(TARGET ${NAME} PROPERTY FOLDER "plugins")

target_link_libraries(${NAME} core wow Qt5::Core)

set(BIN_DIR "${WMV_BASE_PATH}/bin/plugins/")

if (MSVC_IDE)
	# Enable Qt in Visual Studio
	set_property(TARGET ${NAME} PROPERTY VS_GLOBAL_KEYWORD "Qt4VSv1.0")
	set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BIN_DIR})
	set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BIN_DIR})
	set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${BIN_DIR})
	set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${BIN_DIR})
endif()

if(WIN32)
  install(TARGETS ${NAME} RUNTIME DESTINATION ${BIN_DIR})
endif(WIN32)


        