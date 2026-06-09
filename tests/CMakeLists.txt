find_package(GTest CONFIG REQUIRED)

add_executable(decade_tests
	packages/test_reentry_guard.cpp
	packages/test_date_group_store.cpp
	packages/test_page_setup_store.cpp
	packages/test_title_config_store.cpp
	packages/test_shape_configuration_store.cpp
	packages/test_calendar_config.cpp
	packages/test_date_entry_store.cpp
	packages/test_transform_date_entry.cpp
)

target_compile_features(decade_tests PRIVATE cxx_std_23)

target_include_directories(decade_tests PRIVATE
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/external/sigslot/include
	${Boost_INCLUDE_DIRS}
)

target_link_libraries(decade_tests PRIVATE
	GTest::gtest
	GTest::gtest_main
	${Boost_LIBRARIES}
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
