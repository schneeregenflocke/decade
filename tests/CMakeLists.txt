find_package(GTest REQUIRED)

# This file globs src/ itself rather than borrowing a variable from the parent.
# main.cpp stays out: gtest_main defines the main it would collide with.
file(GLOB_RECURSE decade_units CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)
list(FILTER decade_units EXCLUDE REGEX "/main\\.cpp$")

add_executable(decade_tests
	${decade_units}
	domain/test_date.cpp
	domain/test_date_period.cpp
	domain/test_date_format.cpp
	domain/test_reentry_guard.cpp
	domain/test_date_group_store.cpp
	domain/test_page_setup_store.cpp
	domain/test_title_config_store.cpp
	domain/test_shape_configuration_store.cpp
	domain/test_calendar_config.cpp
	domain/test_timeline_projection.cpp
	domain/test_date_entry_store.cpp
	domain/test_date_entry_list.cpp
	domain/test_transform_date_entry.cpp
	domain/test_text_edit_buffer.cpp
	infrastructure/persistence/test_csv_io.cpp
	infrastructure/persistence/test_project_io.cpp
	infrastructure/persistence/test_value_serialization.cpp
	infrastructure/physics/test_physics_world.cpp
	infrastructure/graphics/test_pan_zoom_camera.cpp
	infrastructure/graphics/test_frame_stats.cpp
	infrastructure/graphics/test_projection.cpp
	infrastructure/graphics/test_child_pool.cpp
	infrastructure/graphics/test_scene_graph.cpp
	application/calendar/test_calendar_layout.cpp
	application/calendar/test_title_text_editor.cpp
	application/test_project_document.cpp
)

target_compile_features(decade_tests PRIVATE cxx_std_26)
set_target_properties(decade_tests PROPERTIES AUTOMOC ON)

target_include_directories(decade_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
# SYSTEM keeps foreign warnings and clang-tidy findings out of the -Werror gate.
target_include_directories(decade_tests SYSTEM PRIVATE
	${EPOXY_INCLUDE_DIRS}
	${ICU_INCLUDE_DIRS}
	${CMAKE_SOURCE_DIR}/external/csv2/include
	${CMAKE_SOURCE_DIR}/external/tinycolormap/include
	${CMAKE_SOURCE_DIR}/external/sigslot/include
	${CMAKE_SOURCE_DIR}/external/bullet3/src
)

target_link_libraries(decade_tests PRIVATE
	GTest::gtest
	GTest::gtest_main
	Boost::serialization
	Boost::wserialization
	Qt6::Widgets
	Qt6::OpenGLWidgets
	Qt6::PrintSupport
	ZLIB::ZLIB
	PNG::PNG
	Fontconfig::Fontconfig
	Freetype::Freetype
	glm::glm
	Microsoft.GSL::GSL
	BulletCollision
	LinearMath
	${EPOXY_LIBRARIES}
	${ICU_LIBRARIES}
)

target_compile_definitions(decade_tests PRIVATE GLM_ENABLE_EXPERIMENTAL)

# epoxy #errors out if Qt's qopengl.h claimed __gl_h_ first, and source order
# cannot settle it because clang-format sorts the includes. AGENTS.md, "OpenGL
# under Qt".
target_compile_options(decade_tests PRIVATE -include epoxy/gl.h)

# Less strict than the executable — tests trade off some warning hygiene for
# brevity (e.g. literal integer narrowing in EXPECT_EQ arguments).
target_compile_options(decade_tests PRIVATE -Wall -Wextra -Wpedantic)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	# Clang 22 grades #embed as an extension, and the units under test carry it.
	target_compile_options(decade_tests PRIVATE -Wno-c23-extensions)
endif()

include(GoogleTest)
gtest_discover_tests(decade_tests)
