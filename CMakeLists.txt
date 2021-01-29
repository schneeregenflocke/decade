cmake_minimum_required(VERSION 3.16.0)
message("CMAKE_VERSION: ${CMAKE_VERSION}")


project(Decade_Project VERSION 5.0.1)
message("CMAKE_PROJECT_VERSION: ${CMAKE_PROJECT_VERSION}")
message("CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
message("CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")
message("CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
message("CMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}")
message("CMAKE_CFG_INTDIR: ${CMAKE_CFG_INTDIR}")
#message("CMAKE_CXX_COMPILE_FEATURES: ${CMAKE_CXX_COMPILE_FEATURES}")


set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "CMAKE_EXPORT_COMPILE_COMMANDS" FORCE)


add_executable(Decade)

target_compile_features(Decade PUBLIC cxx_std_17)
get_target_property(decade_target_cxx_standard Decade CXX_STANDARD)
message("decade_target_cxx_standard: ${decade_target_cxx_standard}")

add_compile_definitions(HAVE_TYPE_TRAITS)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
	add_compile_options(/Zc:__cplusplus)
endif()


set(SOURCE_FILES_DECADE
	src/main.cpp
	src/calendar_view.h
	src/casts.h
	src/date_utils.h
	src/packages/calendar_config.h
	src/packages/date_store.h
	src/packages/group_store.h
	src/packages/page_config.h
	src/packages/shape_config.h
	src/packages/title_config.h
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
)


### qt ###

#set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTORCC ON)
#set(CMAKE_AUTOUIC ON)

set(qt_installation_prefix CACHE PATH "Set your CMAKE_PREFIX_PATH environment variable to the Qt 5 installation prefix (e.g. ../Qt/6.0.0/msvc2019_64)")
list(APPEND CMAKE_PREFIX_PATH ${qt_installation_prefix})

find_package(Qt6 COMPONENTS Widgets REQUIRED)

target_link_libraries(Decade PRIVATE Qt6::Widgets)


add_custom_command(
    TARGET Decade
    POST_BUILD
    COMMAND ${qt_installation_prefix}/bin/qtenv2.bat 
    COMMAND ${qt_installation_prefix}/bin/windeployqt.exe ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
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

target_include_directories(Decade PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/external/freetype2/include")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/freetype2")
target_link_libraries(Decade PRIVATE freetype)


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

#add_custom_command(
#    OUTPUT LICENSE.GPLv3
#    PRE_BUILD
#    COMMAND wget "https://code.qt.io/cgit/qt/qt5.git/plain/LICENSE.GPLv3?h=6.0" -O ${CMAKE_CURRENT_BINARY_DIR}/LICENSE.GPLv3
#    )


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
	#list(APPEND LICENSE_FILES_PATHS "${qt_installation_prefix}/../Src/LICENSE.LGPLv3" qt)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
	list(APPEND LICENSE_FILES_PATHS "/usr/share/licenses/boost/LICENSE_1_0.txt" boost)
endif()

embed_resources(SHADERS ${SHADER_FILES_PATHS})
embed_resources(RESOURCES ${LICENSE_FILES_PATHS})

message("RESOURCES: ${RESOURCES}")

target_sources(Decade PRIVATE ${SHADERS} ${RESOURCES})



### sources ###
qt_wrap_cpp(QT_MOC_FILES ${SOURCE_FILES_QT_GUI})
target_sources(Decade PRIVATE ${SOURCE_FILES_DECADE} ${SOURCE_FILES_QT_GUI} ${QT_MOC_FILES})

message("SOURCE_FILES_DECADE: ${SOURCE_FILES_DECADE}")
message("SOURCE_FILES_QT_GUI: ${SOURCE_FILES_QT_GUI}")
message("QT_MOC_FILES: ${QT_MOC_FILES}")


### CPACK
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
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/License.txt")
set(CPACK_PACKAGE_VERSION_MAJOR "5")
set(CPACK_PACKAGE_VERSION_MINOR "0")
include(CPack)
