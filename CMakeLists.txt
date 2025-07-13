cmake_minimum_required(VERSION 3.15)

set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE PATH "Path to your vcpkg installation" FORCE)
# set(VCPKG_ROOT "$ENV{VCPKG_ROOT}" CACHE PATH "Path to your vcpkg installation" FORCE)
# set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "Target triplet for vcpkg" FORCE)
# set(VCPKG_MANIFEST_MODE OFF CACHE BOOL "Enable vcpkg manifest mode" FORCE)


include(CMakePrintHelpers)

project(decade VERSION 0.0.0)

cmake_print_variables(CMAKE_VERSION CMAKE_PROJECT_VERSION CMAKE_SYSTEM_NAME CMAKE_CXX_COMPILER CMAKE_CXX_COMPILER_ID)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Generate compile_commands.json for clang tooling" FORCE)

cmake_print_variables(CMAKE_TOOLCHAIN_FILE VCPKG_ROOT VCPKG_INSTALLED_DIR VCPKG_TARGET_TRIPLET)

add_executable(${PROJECT_NAME} WIN32)
#add_executable(${PROJECT_NAME})


target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
get_target_property(${PROJECT_NAME}_CXX_STANDARD ${PROJECT_NAME} CXX_STANDARD)
cmake_print_variables(${PROJECT_NAME}_CXX_STANDARD CMAKE_CXX_STANDARD CMAKE_CXX_EXTENSIONS)
cmake_print_variables(CMAKE_EXPORT_COMPILE_COMMANDS)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /Zc:__cplusplus)
endif()


target_sources(${PROJECT_NAME} PRIVATE 
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/main_window.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/date_utils.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/casts.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/calendar_page.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/frame_layout.hpp
#	${CMAKE_CURRENT_SOURCE_DIR}/src/wx_widgets_include.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/packages/calendar_config.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/packages/date_store.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/packages/group_store.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/packages/page_config.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/packages/shape_config.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/packages/title_config.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/calendar_panel.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/date_panel.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/font_panel.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/groups_panel.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/license_panel.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/opengl_panel.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/page_panel.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/shape_panel.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/title_panel.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/log_panel.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/font.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/graphics_engine.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/projection.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/rect.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/render_to_texture.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/render_to_png.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/shaders.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/shaders_info.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/shapes.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/shapes_base.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/texture_object.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/mvp_matrices.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/scene_graph.hpp
)


find_package(Boost CONFIG REQUIRED COMPONENTS date_time serialization wserialization)
cmake_print_variables(Boost_FOUND Boost_INCLUDE_DIRS Boost_LIBRARY_DIRS Boost_LIBRARIES Boost_USE_STATIC_LIBS)
if(${Boost_FOUND})
	target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
	target_link_libraries(${PROJECT_NAME} PRIVATE ${Boost_LIBRARIES})
endif()


find_package(OpenGL REQUIRED)


find_package(wxWidgets CONFIG REQUIRED COMPONENTS propgrid gl core base)
target_link_libraries(${PROJECT_NAME} PRIVATE ${wxWidgets_LIBRARIES})
cmake_print_variables(wxWidgets_FOUND wxWidgets_INCLUDE_DIRS wxWidgets_LIBRARIES wxWidgets_LIBRARY_DIRS wxWidgets_DEFINITIONS wxWidgets_DEFINITIONS_DEBUG wxWidgets_CXX_FLAGS wxWidgets_USE_FILE)


target_sources(${PROJECT_NAME} PRIVATE ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/wx/msw/wx_dpi_aware_pmv2.manifest)


find_package(ZLIB REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE ZLIB::ZLIB)


find_package(PNG REQUIRED)
#target_link_libraries(${PROJECT_NAME} PRIVATE PNG::PNG_STATIC)
target_link_libraries(${PROJECT_NAME} PRIVATE PNG::PNG)

find_package(Fontconfig REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Fontconfig::Fontconfig)

find_package(freetype CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE freetype)


set(GLAD_PROFILE "core" CACHE STRING "OpenGL profile" FORCE)
find_package(glad CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glad::glad)


find_package(glm CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)
#target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm-header-only)


find_path(P_RANAV_CSV2_INCLUDE_DIRS "csv2/mio.hpp")
target_include_directories(${PROJECT_NAME} PRIVATE ${P_RANAV_CSV2_INCLUDE_DIRS})
cmake_print_variables(P_RANAV_CSV2_INCLUDE_DIRS)


target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/sigslot/include)

find_package(tabulate CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE tabulate::tabulate)

find_package(Microsoft.GSL CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Microsoft.GSL::GSL)


add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/embed-resource)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/embed-resource)


list(APPEND RESOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/font_fragment_shader.glsl shader
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/font_vertex_shader.glsl shader
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/phong_fragment_shader.glsl shader
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/phong_vertex_shader.glsl shader
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/simple_fragment_shader.glsl shader
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/simple_vertex_shader.glsl shader
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/rectangles_fragment_shader.glsl shader
	${CMAKE_CURRENT_SOURCE_DIR}/shaders/rectangles_vertex_shader.glsl shader
)

list(APPEND RESOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt decade

	${CMAKE_CURRENT_SOURCE_DIR}/external/embed-resource/LICENSE.txt embed-resource	 
	${CMAKE_CURRENT_SOURCE_DIR}/external/sigslot/LICENSE sigslot

	${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/boost-date-time/copyright boost
	${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/freetype/copyright freetype
	${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/zlib/copyright zlib
	${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/libpng/copyright libpng
	${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/p-ranav-csv2/copyright csv2
	${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/p-ranav-csv2/LICENSE.mio csv2mio
	${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/glad/copyright glad
	${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/glm/copyright glm
	${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/wxwidgets/copyright wxwidgets
	${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/fontconfig/copyright fontconfig
	${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/opengl/copyright opengl
	${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/tabulate/copyright tabulate
)

embed_resources(${PROJECT_NAME}_RESOURCES ${RESOURCE_FILES})
cmake_print_variables(${PROJECT_NAME}_RESOURCES)
target_sources(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_RESOURCES})
