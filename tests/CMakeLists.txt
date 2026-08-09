find_package(GTest REQUIRED)

add_executable(decade_tests
	${decade_sources}
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

target_link_libraries(decade_tests PRIVATE
	decade_dependencies
	GTest::gtest
	GTest::gtest_main
)
set_target_properties(decade_tests PROPERTIES AUTOMOC ON)

# Less strict than the executable — tests trade off some warning hygiene for
# brevity (e.g. literal integer narrowing in EXPECT_EQ arguments).
target_compile_options(decade_tests PRIVATE -Wall -Wextra -Wpedantic)

include(GoogleTest)
gtest_discover_tests(decade_tests)
