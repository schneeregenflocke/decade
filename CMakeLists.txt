cmake_minimum_required(VERSION 3.15)

include(CMakePrintHelpers)

project(decade VERSION 0.0.0)

cmake_print_variables(CMAKE_VERSION CMAKE_PROJECT_VERSION CMAKE_SYSTEM_NAME CMAKE_CXX_COMPILER CMAKE_CXX_COMPILER_ID)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Generate compile_commands.json for clang tooling" FORCE)

add_executable(${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)
get_target_property(${PROJECT_NAME}_CXX_STANDARD ${PROJECT_NAME} CXX_STANDARD)
cmake_print_variables(${PROJECT_NAME}_CXX_STANDARD CMAKE_CXX_STANDARD CMAKE_CXX_EXTENSIONS)
cmake_print_variables(CMAKE_EXPORT_COMPILE_COMMANDS)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	target_compile_options(${PROJECT_NAME} PRIVATE
		-Wall
		-Wextra
		-Wpedantic
		-Wshadow
		-Wnon-virtual-dtor
		-Woverloaded-virtual
		-Wold-style-cast
		-Wcast-align
		-Wnull-dereference
		-Wdouble-promotion
		-Wformat=2
		-Wmisleading-indentation
		-Wimplicit-fallthrough
		-Wconversion
		-Wsign-conversion
		-Wzero-as-null-pointer-constant
		-Wundef
		-Wfloat-conversion
		-Werror
	)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(${PROJECT_NAME} PRIVATE
		-Wlogical-op
		-Wduplicated-cond
		-Wduplicated-branches
		-Wuseless-cast
	)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	target_compile_options(${PROJECT_NAME} PRIVATE
		-Wunreachable-code
		-Wextra-semi
	)
endif()

target_sources(${PROJECT_NAME} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src/app/main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/app/decade_app.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/app/app_config.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/app/main_window.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/app/services/project_io.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/app/binding/calendar_page.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/app/binding/calendar_scene_builder.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/app/binding/event_bus.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/app/binding/main_window_binder.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/date_utils.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/casts.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/frame_layout.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/packages/calendar_config.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/packages/color_palette.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/packages/date_store.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/packages/group_store.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/packages/page_config.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/packages/shape_config.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/packages/title_config.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/packages/detail/reentry_guard.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/calendar_panel.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/date_panel.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/font_panel.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/groups_panel.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/license_panel.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/mouse_interaction.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/opengl_panel.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/page_panel.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/shape_panel.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gui/title_panel.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/graphics/debug_log.hpp
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

find_package(PkgConfig REQUIRED)
pkg_check_modules(EPOXY REQUIRED epoxy)
target_include_directories(${PROJECT_NAME} PRIVATE ${EPOXY_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${EPOXY_LIBRARIES})
target_compile_options(${PROJECT_NAME} PRIVATE ${EPOXY_CFLAGS_OTHER})

find_package(wxWidgets CONFIG REQUIRED COMPONENTS propgrid gl core base)
target_link_libraries(${PROJECT_NAME} PRIVATE ${wxWidgets_LIBRARIES})
cmake_print_variables(wxWidgets_FOUND wxWidgets_INCLUDE_DIRS wxWidgets_LIBRARIES wxWidgets_LIBRARY_DIRS wxWidgets_DEFINITIONS wxWidgets_DEFINITIONS_DEBUG wxWidgets_CXX_FLAGS wxWidgets_USE_FILE)


find_package(ZLIB REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE ZLIB::ZLIB)

find_package(PNG REQUIRED)
#target_link_libraries(${PROJECT_NAME} PRIVATE PNG::PNG_STATIC)
target_link_libraries(${PROJECT_NAME} PRIVATE PNG::PNG)

find_package(Fontconfig REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Fontconfig::Fontconfig)

find_package(Freetype REQUIRED)
if(TARGET freetype)
	target_link_libraries(${PROJECT_NAME} PRIVATE freetype)
elseif(TARGET Freetype::Freetype)
	target_link_libraries(${PROJECT_NAME} PRIVATE Freetype::Freetype)
else()
	target_include_directories(${PROJECT_NAME} PRIVATE ${FREETYPE_INCLUDE_DIRS})
	target_link_libraries(${PROJECT_NAME} PRIVATE ${FREETYPE_LIBRARIES})
endif()

find_package(glm CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)
target_compile_definitions(${PROJECT_NAME} PRIVATE GLM_ENABLE_EXPERIMENTAL)
#target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm-header-only)

set(CSV2_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/csv2/include")
if(EXISTS "${CSV2_INCLUDE_DIR}/csv2/reader.hpp")
	target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${CSV2_INCLUDE_DIR})
else()
	find_path(P_RANAV_CSV2_INCLUDE_DIRS "csv2/mio.hpp")
	if(P_RANAV_CSV2_INCLUDE_DIRS)
		target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${P_RANAV_CSV2_INCLUDE_DIRS})
		cmake_print_variables(P_RANAV_CSV2_INCLUDE_DIRS)
	else()
		message(FATAL_ERROR "csv2 not found. Initialize submodule external/csv2 or provide csv2 include dir.")
	endif()
endif()

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/sigslot/include)

set(TINYCOLORMAP_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/tinycolormap/include")
if(NOT EXISTS "${TINYCOLORMAP_INCLUDE_DIR}/tinycolormap.hpp")
	message(FATAL_ERROR "tinycolormap not found. Initialize submodule external/tinycolormap (git submodule update --init --recursive).")
endif()
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${TINYCOLORMAP_INCLUDE_DIR})

find_package(Microsoft.GSL CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Microsoft.GSL::GSL)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/embed-resource)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/embed-resource)

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
)

list(APPEND RESOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/external/embed-resource/LICENSE.txt embed_resource_LICENSE
	${CMAKE_CURRENT_SOURCE_DIR}/external/sigslot/LICENSE sigslot_LICENSE
	${CMAKE_CURRENT_SOURCE_DIR}/external/csv2/LICENSE csv2_copyright
	${CMAKE_CURRENT_SOURCE_DIR}/external/csv2/LICENSE.mio csv2mio_LICENSE
	${CMAKE_CURRENT_SOURCE_DIR}/external/tinycolormap/LICENSE tinycolormap_LICENSE
)

embed_resources(${PROJECT_NAME}_RESOURCES ${RESOURCE_FILES})
cmake_print_variables(${PROJECT_NAME}_RESOURCES)
target_sources(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_RESOURCES})

option(DECADE_BUILD_TESTS "Build unit tests for src/packages/" ON)
if(DECADE_BUILD_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif()
