cmake_minimum_required(VERSION 3.15)

#set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/external/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")

include(CMakePrintHelpers)

#-DUSE_MSVC_RUNTIME_LIBRARY_DLL=false
#-DVCPKG_TARGET_TRIPLET=x64-windows-static-md
#cmake_policy(SET CMP0091 NEW)
#set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(Decade VERSION 0.0.0)

cmake_print_variables(CMAKE_VERSION CMAKE_PROJECT_VERSION CMAKE_SYSTEM_NAME CMAKE_CXX_COMPILER CMAKE_CXX_COMPILER_ID)

#add_executable(${PROJECT_NAME} WIN32)
add_executable(${PROJECT_NAME} WIN32)

#set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
get_target_property(${PROJECT_NAME}_CXX_STANDARD ${PROJECT_NAME} CXX_STANDARD)
cmake_print_variables(${PROJECT_NAME}_CXX_STANDARD CMAKE_CXX_STANDARD CMAKE_CXX_EXTENSIONS)
cmake_print_variables(CMAKE_EXPORT_COMPILE_COMMANDS)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /Zc:__cplusplus)
endif()

get_target_property(${PROJECT_NAME}_COMPILE_OPTIONS ${PROJECT_NAME} COMPILE_OPTIONS)
cmake_print_variables(${PROJECT_NAME}_COMPILE_OPTIONS)

get_target_property(${PROJECT_NAME}_COMPILE_DEFINITIONS ${PROJECT_NAME} COMPILE_DEFINITIONS)
cmake_print_variables(${PROJECT_NAME}_COMPILE_DEFINITIONS)

target_sources(${PROJECT_NAME} PRIVATE 
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/date_utils.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/casts.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/calendar_page.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/frame_layout.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/packages/calendar_config.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/packages/date_store.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/packages/group_store.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/packages/page_config.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/packages/shape_config.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/packages/title_config.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/main_window.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/calendar_panel.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/date_panel.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/font_panel.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/groups_panel.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/license_panel.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/opengl_panel.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/page_panel.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/shape_panel.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/title_panel.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/wx_widgets_include.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/log_panel.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/font.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/graphics_engine.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/projection.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/rect.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/render_to_texture.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/render_to_png.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/shaders.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/shapes.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/shapes_base.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/texture_object.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/mvp_matrices.hpp
)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED COMPONENTS date_time serialization wserialization)
cmake_print_variables(Boost_FOUND Boost_INCLUDE_DIRS Boost_LIBRARY_DIRS Boost_LIBRARIES Boost_USE_STATIC_LIBS)
if(${Boost_FOUND})
	target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
	target_link_libraries(${PROJECT_NAME} PRIVATE ${Boost_LIBRARIES})
endif()


find_package(OpenGL REQUIRED)


#find_package(wxWidgets CONFIG REQUIRED)
find_package(wxWidgets CONFIG REQUIRED COMPONENTS propgrid gl core base)
include("${wxWidgets_USE_FILE}")
#target_link_libraries(${PROJECT_NAME} PRIVATE wx::propgrid wx::gl wx::core wx::base)
target_link_libraries(${PROJECT_NAME} PRIVATE ${wxWidgets_LIBRARIES})
#cmake_print_variables(wxWidgets_USE_FILE wxWidgets_LIBRARIES)
cmake_print_variables(wxWidgets_FOUND wxWidgets_INCLUDE_DIRS wxWidgets_LIBRARIES wxWidgets_LIBRARY_DIRS wxWidgets_DEFINITIONS wxWidgets_DEFINITIONS_DEBUG wxWidgets_CXX_FLAGS wxWidgets_USE_FILE)

get_target_property(WX_BASE_INCLUDE_DIRECTORIES wx::base INTERFACE_INCLUDE_DIRECTORIES)
cmake_print_variables(WX_BASE_INCLUDE_DIRECTORIES)
cmake_print_variables(VCPKG_ROOT)
get_filename_component(CMAKE_TOOLCHAIN_FILE_DIRECTORY "${CMAKE_TOOLCHAIN_FILE}" DIRECTORY)
cmake_print_variables(CMAKE_TOOLCHAIN_FILE_DIRECTORY)
#target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_TOOLCHAIN_FILE_DIRECTORY}/../../installed/x64-windows/include/wx/msw/wx_dpi_aware.manifest)
target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_TOOLCHAIN_FILE_DIRECTORY}/../../installed/x64-windows/include/wx/msw/wx_dpi_aware_pmv2.manifest)
#wx_dpi_aware_pmv2.manifest
#find_package(wxWidgets REQUIRED propgrid gl core base)
#cmake_print_variables(wxWidgets_FOUND wxWidgets_INCLUDE_DIRS wxWidgets_LIBRARIES wxWidgets_LIBRARY_DIRS wxWidgets_DEFINITIONS wxWidgets_DEFINITIONS_DEBUG wxWidgets_CXX_FLAGS wxWidgets_USE_FILE)
#if(wxWidgets_FOUND)
#  include(${wxWidgets_USE_FILE})
#  target_include_directories(${PROJECT_NAME} PRIVATE ${wxWidgets_INCLUDE_DIRS})
#  target_link_libraries(${PROJECT_NAME} PRIVATE ${wxWidgets_LIBRARIES})
#endif()

#set(wxUSE_ZLIB ON CACHE BOOL "wxUSE_STL" FORCE)
#set(wxUSE_LIBPNG ON CACHE BOOL "wxUSE_STL" FORCE)
#set(wxUSE_STL ON CACHE BOOL "wxUSE_STL" FORCE)
#set(wxUSE_GUI ON CACHE BOOL "wxUSE_GUI" FORCE)
#set(wxUSE_OPENGL ON CACHE BOOL "wxUSE_OPENGL" FORCE)
#set(wxUSE_STD_IOSTREAM ON CACHE BOOL "wxUSE_STD_IOSTREAM" FORCE)
#set(wxUSE_WINRT	OFF CACHE BOOL "wxUSE_WINRT" FORCE)
#set(wxUSE_EXCEPTIONS OFF CACHE BOOL "wxUSE_EXCEPTIONS" FORCE)
#set(wxUSE_GLCANVAS_EGL OFF CACHE BOOL "wxUSE_GLCANVAS_EGL" FORCE)
#set(wxBUILD_SHARED OFF CACHE BOOL "wxBUILD_SHARED" FORCE)
#add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/wxWidgets")
#target_link_libraries(${PROJECT_NAME} PRIVATE wxpropgrid wxgl wxcore wxbase)
#if(${CMAKE_SYSTEM_NAME} STREQUAL Windows)
#	target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/wxWidgets/include/wx/msw/wx_dpi_aware.manifest)
#endif()

find_package(ZLIB REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE ZLIB::ZLIB)

#find_package(libpng REQUIRED)
#target_link_libraries(${PROJECT_NAME} PRIVATE png_static) #png_static
find_package(PNG REQUIRED)
#target_link_libraries(${PROJECT_NAME} PRIVATE PNG::PNG_STATIC)
target_link_libraries(${PROJECT_NAME} PRIVATE PNG::PNG)

find_package(freetype CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE freetype)

set(GLAD_PROFILE "core" CACHE STRING "OpenGL profile" FORCE)
find_package(glad CONFIG REQUIRED)
#glad_add_library(glad_gl_core_46 REPRODUCIBLE API gl:core=4.6)
target_link_libraries(${PROJECT_NAME} PRIVATE glad::glad)
#target_link_libraries(${PROJECT_NAME} PRIVATE glad_gl_core_46)

#set(GLAD_PROFILE "core" CACHE STRING "OpenGL profile" FORCE)
#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/glad/cmake)

#target_link_libraries(${PROJECT_NAME} PRIVATE glad_gl_core_46)

find_package(glm CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)

    # Or use the header-only version
    #find_package(glm CONFIG REQUIRED)
    #target_link_libraries(main PRIVATE glm::glm-header-only)
#add_compile_definitions(GLM_FORCE_CXX17)
#target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/glm)

find_path(P_RANAV_CSV2_INCLUDE_DIRS "csv2/mio.hpp")
target_include_directories(${PROJECT_NAME} PRIVATE ${P_RANAV_CSV2_INCLUDE_DIRS})
cmake_print_variables(P_RANAV_CSV2_INCLUDE_DIRS)

#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/csv2)
#target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/csv2/include)


target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/sigslot/include)


target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/embed-resource)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/embed-resource)


list(APPEND RESOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/font_fragment_shader.glsl shader
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/font_vertex_shader.glsl shader
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/phong_fragment_shader.glsl shader
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/phong_vertex_shader.glsl shader
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/simple_fragment_shader.glsl shader
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/simple_vertex_shader.glsl shader
)

list(APPEND RESOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt decade

	${CMAKE_CURRENT_SOURCE_DIR}/external/embed-resource/LICENSE.txt embed-resource	 
	${CMAKE_CURRENT_SOURCE_DIR}/external/sigslot/LICENSE sigslot

	#${CMAKE_CURRENT_SOURCE_DIR}/external/csv2/LICENSE csv2
	#${CMAKE_CURRENT_SOURCE_DIR}/external/csv2/LICENSE.mio csv2mio
	#${CMAKE_CURRENT_SOURCE_DIR}/external/glad/LICENSE glad
	#${CMAKE_CURRENT_SOURCE_DIR}/external/glm/copying.txt glm
	#${CMAKE_CURRENT_SOURCE_DIR}/external/wxWidgets/docs/licence.txt wxwidgets

	${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/boost-date-time/copyright boost
	${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/freetype/copyright freetype
	${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/zlib/copyright zlib
	${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/libpng/copyright libpng

	${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/p-ranav-csv2/copyright csv2
	${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/p-ranav-csv2/LICENSE.mio csv2mio
	${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/glad/copyright glad
	${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/glm/copyright glm
	${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/wxwidgets/copyright wxwidgets
)

embed_resources(${PROJECT_NAME}_RESOURCES ${RESOURCE_FILES})
cmake_print_variables(${PROJECT_NAME}_RESOURCES)
target_sources(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_RESOURCES})
