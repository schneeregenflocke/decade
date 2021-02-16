cmake_minimum_required(VERSION 3.16.0)
message("CMAKE_VERSION: ${CMAKE_VERSION}")


project(Decade_Project VERSION 5.0.1)


message("CMAKE_PROJECT_VERSION: ${CMAKE_PROJECT_VERSION}")
message("CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
message("CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")
message("CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
message("CMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}")
message("CMAKE_CFG_INTDIR: ${CMAKE_CFG_INTDIR}")
message("CMAKE_CXX_COMPILE_FEATURES: ${CMAKE_CXX_COMPILE_FEATURES}")


set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "CMAKE_EXPORT_COMPILE_COMMANDS" FORCE)


add_executable(Decade)

target_compile_features(Decade PUBLIC cxx_std_17)
get_target_property(DECADE_TARGET_CXX_STANDARD Decade CXX_STANDARD)
message("DECADE_TARGET_CXX_STANDARD: ${DECADE_TARGET_CXX_STANDARD}")

#add_compile_definitions(HAVE_TYPE_TRAITS)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
	add_compile_options(/Zc:__cplusplus)
endif()





### qt ###

#set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTORCC ON)
#set(CMAKE_AUTOUIC ON)

set(QT_INSTALLATION_PREFIX CACHE PATH "Set your CMAKE_PREFIX_PATH environment variable to the Qt installation prefix (e.g. /Qt/6.0.0/msvc2019_64)")
list(APPEND CMAKE_PREFIX_PATH ${QT_INSTALLATION_PREFIX})
message("QT_INSTALLATION_PREFIX: ${QT_INSTALLATION_PREFIX}")
message("CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

find_package(Qt6 COMPONENTS Widgets REQUIRED)

target_link_libraries(Decade PRIVATE Qt6::Widgets)


add_custom_command(
    TARGET Decade
    POST_BUILD
    COMMAND ${QT_INSTALLATION_PREFIX}/bin/qtenv2.bat 
    COMMAND ${QT_INSTALLATION_PREFIX}/bin/windeployqt.exe ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
    )


### boost ###

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS date_time serialization wserialization)

message("Boost_FOUND: ${Boost_FOUND}")
if(NOT ${Boost_FOUND})
	message(FATAL_ERROR "Set directory path variable Boost_DIR to the directory containing the file BoostConfig.cmake (e.g. boost/stage/lib/cmake/Boost-x.xx.x)")
endif()

message("Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
message("Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")
message("Boost_LIBRARIES: ${Boost_LIBRARIES}")

target_include_directories(Decade PRIVATE ${Boost_INCLUDE_DIRS})
#if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
#	target_link_directories(Decade PRIVATE ${Boost_LIBRARY_DIRS})
#endif()
target_link_libraries(Decade PRIVATE ${Boost_LIBRARIES})


### glad ###
set(GLAD_PROFILE "core" CACHE STRING "OpenGL profile" FORCE)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/glad")
target_link_libraries(Decade PRIVATE glad)


### glm ###
#add_compile_definitions(GLM_FORCE_MESSAGES)
add_compile_definitions(GLM_FORCE_CXX17)
target_include_directories(Decade PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/external/glm")


### freetype ###

#set(FT_WITH_BROTLI OFF CACHE BOOL "FT_WITH_BROTLI" FORCE)
#set(FT_WITH_BZIP2 OFF CACHE BOOL "FT_WITH_BZIP2" FORCE)
#set(FT_WITH_HARFBUZZ OFF CACHE BOOL "FT_WITH_HARFBUZZ" FORCE)
#set(FT_WITH_PNG OFF CACHE BOOL "FT_WITH_PNG" FORCE)
#set(FT_WITH_ZLIB OFF CACHE BOOL "FT_WITH_ZLIB" FORCE)

set(FREETYPE_FROM_SUBDIRECTORY ON CACHE BOOL "add freetype from subdirectory.")
message("FREETYPE_FROM_SUBDIRECTORY: ${FREETYPE_FROM_SUBDIRECTORY}")

if(${FREETYPE_FROM_SUBDIRECTORY})
	target_include_directories(Decade PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/external/freetype2/include")
	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/freetype2")
	target_link_libraries(Decade PRIVATE freetype)
else()
	set(FREETYPE_DIR CACHE PATH "The user may set the environment variable FREETYPE_DIR to the root directory of a Freetype installation.")
	find_package(Freetype REQUIRED)
	message("FREETYPE_FOUND: ${FREETYPE_FOUND}")
	target_include_directories(Decade PRIVATE ${FREETYPE_INCLUDE_DIRS})
	target_link_libraries(Decade PRIVATE ${FREETYPE_LIBRARIES})
endif()


### lodepng ###
target_include_directories(Decade PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/external/lodepng")
target_sources(Decade PRIVATE external/lodepng/lodepng.h external/lodepng/lodepng.cpp)


### embed-resource ###
target_include_directories(Decade PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/external/embed-resource")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/embed-resource")


### sigslot ###
target_include_directories(Decade PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/external/sigslot/include")



list(APPEND SHADER_FILES_PATHS
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/font_fragment_shader.glsl shader
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/font_vertex_shader.glsl shader
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/phong_fragment_shader.glsl shader
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/phong_vertex_shader.glsl shader
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/simple_fragment_shader.glsl shader
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/simple_vertex_shader.glsl shader
)

file(DOWNLOAD https://code.qt.io/cgit/qt/qt5.git/plain/LICENSE.GPLv3?h=6.0 ${CMAKE_CURRENT_BINARY_DIR}/LICENSE.GPLv3)


list(APPEND LICENSE_FILES_PATHS
	"${CMAKE_CURRENT_BINARY_DIR}/LICENSE.GPLv3" qt
	"${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt" decade
	"${CMAKE_CURRENT_SOURCE_DIR}/external/freetype2/docs/LICENSE.TXT" freetype2
	"${CMAKE_CURRENT_SOURCE_DIR}/external/embed-resource/LICENSE.txt" "embed-resource"		 
	"${CMAKE_CURRENT_SOURCE_DIR}/external/glad/LICENSE" glad
	"${CMAKE_CURRENT_SOURCE_DIR}/external/glm/copying.txt" glm
	"${CMAKE_CURRENT_SOURCE_DIR}/external/lodepng/LICENSE" lodepng
	"${CMAKE_CURRENT_SOURCE_DIR}/external/sigslot/LICENSE" sigslot
)

if(${CMAKE_SYSTEM_NAME} STREQUAL Windows)
	list(APPEND LICENSE_FILES_PATHS "${Boost_INCLUDE_DIRS}/LICENSE_1_0.txt" boost)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
	list(APPEND LICENSE_FILES_PATHS "/usr/share/licenses/boost/LICENSE_1_0.txt" boost)
endif()

embed_resources(SHADERS ${SHADER_FILES_PATHS})
embed_resources(RESOURCES ${LICENSE_FILES_PATHS})
message("RESOURCES: ${RESOURCES}")
target_sources(Decade PRIVATE ${SHADERS} ${RESOURCES})


### sources ###

set(SOURCE_FILES_DECADE
	src/main.cpp
	src/calendar_view.h
	src/casts.h
	src/date_utils.h
	src/signal_packages/calendar_config.h
	src/signal_packages/date_store.h
	src/signal_packages/group_store.h
	src/signal_packages/page_config.h
	src/signal_packages/shape_config.h
	src/signal_packages/title_config.h
	src/graphics_engine/font.h
	src/graphics_engine/graphics_engine.h
	src/graphics_engine/projection.h
	src/graphics_engine/rect4.h
	src/graphics_engine/render_to_texture.h
	src/graphics_engine/shaders.h
	src/graphics_engine/shapes.h
	src/graphics_engine/shapes_base.h
	src/graphics_engine/texture_object.h
)

set(SOURCE_FILES_QT_GUI
	src/gui/main_window.h
	src/gui/license_info_dialog.h
	src/gui/date_table.h
	src/gui/tools_tabs.h
)

qt6_wrap_cpp(SOURCE_FILES_MOC_FILES ${SOURCE_FILES_QT_GUI})
target_sources(Decade PRIVATE ${SOURCE_FILES_DECADE} ${SOURCE_FILES_MOC_FILES})

message("SOURCE_FILES_DECADE: ${SOURCE_FILES_DECADE}")
message("SOURCE_FILES_QT_GUI: ${SOURCE_FILES_QT_GUI}")
message("SOURCE_FILES_MOC_FILES: ${SOURCE_FILES_MOC_FILES}")



### CPACK ###

#include(GNUInstallDirs)

install(
	TARGETS Decade
	#EXPORT Decade_Project 
	DESTINATION bin
	COMPONENT executable
	)

get_cmake_property(CPACK_COMPONENTS_ALL COMPONENTS)
list(REMOVE_ITEM CPACK_COMPONENTS_ALL "Unspecified" "headers")
message("CPACK_COMPONENTS_ALL: ${CPACK_COMPONENTS_ALL}")

#include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_PACKAGE_VERSION_MAJOR "5")
set(CPACK_PACKAGE_VERSION_MINOR "0")
include(CPack)
