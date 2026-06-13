find_package(GTest CONFIG REQUIRED)

add_executable(decade_tests
	packages/test_date.cpp
	packages/test_date_period.cpp
	packages/test_date_format.cpp
	packages/test_reentry_guard.cpp
	packages/test_date_group_store.cpp
	packages/test_page_setup_store.cpp
	packages/test_title_config_store.cpp
	packages/test_shape_configuration_store.cpp
	packages/test_calendar_config.cpp
	packages/test_timeline_projection.cpp
	packages/test_date_entry_store.cpp
	packages/test_transform_date_entry.cpp
	services/test_csv_io.cpp
	services/test_value_serialization.cpp
	physics/test_physics_world.cpp
)

target_compile_features(decade_tests PRIVATE cxx_std_23)

target_include_directories(decade_tests PRIVATE
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/external/sigslot/include
)

# The packages under test are Boost-free; their only third-party dependency is
# the ICU date backend. The services tests additionally exercise CSV I/O
# (csv2, header-only) and the Boost.Serialization XML round trip. All variables
# and imported targets come from the parent scope.
target_include_directories(decade_tests SYSTEM PRIVATE
	${ICU_INCLUDE_DIRS}
	${CSV2_INCLUDE_DIR}
	${CMAKE_SOURCE_DIR}/external/bullet3/src
)

target_link_libraries(decade_tests PRIVATE
	GTest::gtest
	GTest::gtest_main
	glm::glm
	${ICU_LIBRARIES}
	${Boost_LIBRARIES}
	BulletCollision
	LinearMath
)

# Less strict than the main target — tests trade off some warning hygiene for
# brevity (e.g. literal integer narrowing in EXPECT_EQ arguments).
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	target_compile_options(decade_tests PRIVATE
		-Wall -Wextra -Wpedantic
	)
elseif(MSVC)
	target_compile_options(decade_tests PRIVATE /W3)
endif()

include(GoogleTest)
gtest_discover_tests(decade_tests)
