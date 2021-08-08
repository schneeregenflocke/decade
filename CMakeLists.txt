cmake_minimum_required(VERSION 3.8)

include(CMakePrintHelpers)

project(Decade VERSION 5.0.2)

cmake_print_variables(CMAKE_VERSION CMAKE_PROJECT_VERSION CMAKE_SYSTEM_NAME CMAKE_CXX_COMPILER_ID)

add_executable(${PROJECT_NAME})


target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
get_target_property(${PROJECT_NAME}_CXX_STANDARD ${PROJECT_NAME} CXX_STANDARD)
cmake_print_variables(${PROJECT_NAME}_CXX_STANDARD)


add_compile_definitions(HAVE_TYPE_TRAITS)
target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /Zc:__cplusplus)
	target_compile_options(${PROJECT_NAME} PRIVATE /bigobj)
endif()

get_target_property(${PROJECT_NAME}_COMPILE_OPTIONS ${PROJECT_NAME} COMPILE_OPTIONS)
cmake_print_variables(${PROJECT_NAME}_COMPILE_OPTIONS)

get_target_property(${PROJECT_NAME}_COMPILE_DEFINITIONS ${PROJECT_NAME} COMPILE_DEFINITIONS)
cmake_print_variables(${PROJECT_NAME}_COMPILE_DEFINITIONS)


# boost
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS date_time serialization wserialization)

cmake_print_variables(Boost_FOUND)
if(NOT ${Boost_FOUND})
	message(FATAL_ERROR "Set directory path variable Boost_DIR to the directory containing the file BoostConfig.cmake (e.g. boost/stage/lib/cmake/Boost-x.xx.x)")
endif()

cmake_print_variables(Boost_INCLUDE_DIRS Boost_LIBRARY_DIRS Boost_LIBRARIES)
target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${Boost_LIBRARIES})
if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
	target_link_directories(Decade PRIVATE ${Boost_LIBRARY_DIRS})
endif()



# wxWidgets

#find_package(wxWidgets REQUIRED gl core base propgrid)
#include(${wxWidgets_USE_FILE})
# and for each of your dependent executable/library targets:
#target_link_libraries(Decade PRIVATE ${wxWidgets_LIBRARIES})

target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)

# WxWidgets Configuration ON
set(wxUSE_STL ON CACHE BOOL "wxUSE_STL" FORCE)
set(wxUSE_GUI ON CACHE BOOL "wxUSE_GUI" FORCE)
set(wxUSE_OPENGL ON CACHE BOOL "wxUSE_OPENGL" FORCE)

# WxWidgets Configuration OFF
set(wxBUILD_SHARED OFF CACHE BOOL "wxBUILD_SHARED" FORCE)
set(wxUSE_WINRT	OFF CACHE BOOL "wxUSE_WINRT" FORCE)
set(wxUSE_EXCEPTIONS OFF CACHE BOOL "wxUSE_EXCEPTIONS" FORCE)
set(wxUSE_GLCANVAS_EGL OFF CACHE BOOL "wxUSE_GLCANVAS_EGL" FORCE)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/external/wxWidgets)
# fixed library names https://github.com/wxWidgets/wxWidgets/commit/0f806ad96db4e22053769c0b2f3b7be4026ea8de
target_link_libraries(${PROJECT_NAME} PRIVATE wxpropgrid wxgl wxcore wxbase)



# glad
set(GLAD_PROFILE "core" CACHE STRING "OpenGL profile" FORCE)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/external/glad)
target_link_libraries(${PROJECT_NAME} PRIVATE glad)


# glm 
add_compile_definitions(GLM_FORCE_CXX17)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/external/glm)


# freetype
#set(FT_WITH_BROTLI OFF CACHE BOOL "FT_WITH_BROTLI" FORCE)
#set(FT_WITH_BZIP2 OFF CACHE BOOL "FT_WITH_BZIP2" FORCE)
#set(FT_WITH_HARFBUZZ OFF CACHE BOOL "FT_WITH_HARFBUZZ" FORCE)
#set(FT_WITH_PNG OFF CACHE BOOL "FT_WITH_PNG" FORCE)
#set(FT_WITH_ZLIB OFF CACHE BOOL "FT_WITH_ZLIB" FORCE)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/external/freetype2/include)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/external/freetype2)
target_link_libraries(Decade PRIVATE freetype)


# lodepng 
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/external/lodepng)
target_sources(${PROJECT_NAME} PRIVATE external/lodepng/lodepng.h external/lodepng/lodepng.cpp)


# csv
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/external/csv/include)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/external/csv)


# embed-resource
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/external/embed-resource)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/external/embed-resource)


# sigslot
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/external/sigslot/include)



list(APPEND RESOURCE_FILES
	${CMAKE_CURRENT_LIST_DIR}/shaders/font_fragment_shader.glsl shader
	${CMAKE_CURRENT_LIST_DIR}/shaders/font_vertex_shader.glsl shader
	${CMAKE_CURRENT_LIST_DIR}/shaders/phong_fragment_shader.glsl shader
	${CMAKE_CURRENT_LIST_DIR}/shaders/phong_vertex_shader.glsl shader
	${CMAKE_CURRENT_LIST_DIR}/shaders/simple_fragment_shader.glsl shader
	${CMAKE_CURRENT_LIST_DIR}/shaders/simple_vertex_shader.glsl shader
)


list(APPEND RESOURCE_FILES
	${CMAKE_CURRENT_LIST_DIR}/LICENSE.txt decade
	${CMAKE_CURRENT_LIST_DIR}/external/csv/LICENSE csv 
	${CMAKE_CURRENT_LIST_DIR}/external/csv/vendor/Catch2/LICENSE.txt Catch2
	${CMAKE_CURRENT_LIST_DIR}/external/freetype2/docs/LICENSE.TXT freetype2
	${CMAKE_CURRENT_LIST_DIR}/external/embed-resource/LICENSE.txt embed-resource	 
	${CMAKE_CURRENT_LIST_DIR}/external/glad/LICENSE glad
	${CMAKE_CURRENT_LIST_DIR}/external/glm/copying.txt glm
	${CMAKE_CURRENT_LIST_DIR}/external/lodepng/LICENSE lodepng
	${CMAKE_CURRENT_LIST_DIR}/external/sigslot/LICENSE sigslot
	${CMAKE_CURRENT_LIST_DIR}/external/wxWidgets/docs/licence.txt wxWidgets
	${CMAKE_CURRENT_LIST_DIR}/external/wxWidgets/3rdparty/catch/LICENSE.txt catch
	${CMAKE_CURRENT_LIST_DIR}/external/wxWidgets/src/expat/expat/COPYING expat
	${CMAKE_CURRENT_LIST_DIR}/external/wxWidgets/src/jpeg/README libjpeg_turbo
	${CMAKE_CURRENT_LIST_DIR}/external/wxWidgets/src/tiff/COPYRIGHT libtiff
	${CMAKE_CURRENT_LIST_DIR}/external/wxWidgets/src/png/LICENSE libpng
	${CMAKE_CURRENT_LIST_DIR}/external/wxWidgets/src/zlib/README zlib
)

if(${CMAKE_SYSTEM_NAME} STREQUAL Windows)
	list(APPEND RESOURCE_FILES ${Boost_INCLUDE_DIRS}/LICENSE_1_0.txt boost)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
	list(APPEND RESOURCE_FILES /usr/share/licenses/boost/LICENSE_1_0.txt boost)
endif()


embed_resources(${PROJECT_NAME}_RESOURCES ${RESOURCE_FILES})
cmake_print_variables(${PROJECT_NAME}_RESOURCES)


target_sources(${PROJECT_NAME} PRIVATE 
	${${PROJECT_NAME}_RESOURCES}
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/main_window.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/date_utils.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/casts.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/calendar_view.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/packages/calendar_config.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/packages/date_store.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/packages/group_store.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/packages/page_config.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/packages/shape_config.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/packages/title_config.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/calendar_panel.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/date_panel.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/font_panel.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/groups_panel.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/license_panel.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/opengl_panel.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/page_panel.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/shape_panel.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/title_panel.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/wx_widgets_include.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/font.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/graphic_engine.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/projection.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/rect4.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/RenderToTexture.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/shaders.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/shapes.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/shapes_base.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/texture_object.h
)


if(${CMAKE_SYSTEM_NAME} STREQUAL Windows)
	target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/wxWidgets/include/wx/msw/amd64_dpi_aware.manifest)
endif()


### CPACK
#include(GNUInstallDirs)

install(
	TARGETS ${PROJECT_NAME}
	COMPONENT executable
)

get_cmake_property(CPACK_COMPONENTS_ALL COMPONENTS)
list(REMOVE_ITEM CPACK_COMPONENTS_ALL "Unspecified" "headers")
cmake_print_variables(CPACK_COMPONENTS_ALL)

#include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/License.txt)
set(CPACK_PACKAGE_VERSION_MAJOR ${${PROJECT_NAME}_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${${PROJECT_NAME}_VERSION_MINOR})
include(CPack)
