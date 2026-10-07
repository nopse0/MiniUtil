cmake_minimum_required(VERSION 3.24)
message("Using toolchain file ${CMAKE_TOOLCHAIN_FILE}.")

########################################################################################################################
## Define project
########################################################################################################################
project(
        MiniUtil
        VERSION 0.0.1
        DESCRIPTION "Native Skyrim utilities."
        LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

#include(GNUInstallDirs)

add_subdirectory(lib/commonlibsse-ng)
include(${CMAKE_CURRENT_SOURCE_DIR}/lib/commonlibsse-ng/cmake/CommonLibSSE.cmake)

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/version.rc
        @ONLY)

#file(
#	GLOB_RECURSE
#	headers_extern
#	CONFIGURE_DEPENDS
#		"${CMAKE_CURRENT_SOURCE_DIR}/extern/catmenu/*.h"
#		"${CMAKE_CURRENT_SOURCE_DIR}/extern/catmenu/*.hpp"
#)     

#set(headers_extern ${headers_extern}
#   "${CMAKE_CURRENT_SOURCE_DIR}/extern/SlaveTatsNG/include/config.h"
#)

#set(sources_extern
#    "${CMAKE_CURRENT_SOURCE_DIR}/extern/SlaveTatsNG/src/config.cpp"
#)

set(sources
	${CMAKE_CURRENT_BINARY_DIR}/version.rc)

set(headers
	src/PCH.h
    "include/stat_tracker.h"
)

set(sources ${sources}
    src/main.cpp
    src/scanner.cpp
    src/struggle_game.cpp
    src/main_loop.cpp
    src/actor_util.cpp
    src/controller.cpp
    src/sl_scenes.cpp
    src/papyrus_utils.cpp
    src/snippets.cpp
)

########################################################################################################################
## Configure target DLL
########################################################################################################################
#set(CMAKE_PREFIX_PATH
#	${CMAKE_PREFIX_PATH}
#	${CMAKE_CURRENT_SOURCE_DIR}/lib/commonlibsse-ng/cmake
#	)
	

#find_package(CommonLibSSE CONFIG REQUIRED)
find_path(SIMPLEINI_INCLUDE_DIRS "SimpleIni.h")
find_package(nlohmann_json CONFIG REQUIRED)
#find_package(unofficial-hash-library)
#find_package(magic_enum CONFIG REQUIRED)

add_commonlibsse_plugin(${PROJECT_NAME} SOURCES ${headers} ${sources})
add_library("${PROJECT_NAME}::${PROJECT_NAME}" ALIAS "${PROJECT_NAME}")

#target_include_directories(${PROJECT_NAME}
#        PRIVATE
#        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
#        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src>
#		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/cmake>
#        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/cmake>
#        $<INSTALL_INTERFACE:src>
#		)

#target_include_directories(${PROJECT_NAME}
#        PUBLIC
#        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
#		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/extern/MCP>
#        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/extern/SlaveTatsNG/include>
#		${CLIB_UTIL_INCLUDE_DIRS}
#		)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/mcp>
		${SIMPLEINI_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        nlohmann_json::nlohmann_json
		#unofficial::hash-library
        #${CMAKE_CURRENT_SOURCE_DIR}/extern/catmenu/lib/imgui.lib
        )

target_precompile_headers(${PROJECT_NAME}
        PRIVATE
        src/PCH.h)
