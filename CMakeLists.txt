cmake_minimum_required(VERSION 3.16)

project(decade VERSION 0.0.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE decade_headers CONFIGURE_DEPENDS src/*.hpp)
file(GLOB_RECURSE decade_sources CONFIGURE_DEPENDS src/*.cpp)

add_executable(decade ${decade_sources} ${decade_headers})

# --- Dependencies -------------------------------------------------------------

find_package(Boost CONFIG REQUIRED COMPONENTS serialization wserialization)
find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(Fontconfig REQUIRED)
find_package(Freetype REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Microsoft.GSL CONFIG REQUIRED)

find_package(Qt6 CONFIG REQUIRED COMPONENTS Widgets OpenGLWidgets PrintSupport)

find_package(PkgConfig REQUIRED)
pkg_check_modules(EPOXY REQUIRED epoxy)
pkg_check_modules(ICU REQUIRED icu-uc icu-i18n)

# --- Submodules ----------------------------------------------

set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_CPU_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_BULLET3 OFF CACHE BOOL "" FORCE)
set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_ENET OFF CACHE BOOL "" FORCE)
set(BUILD_CLSOCKET OFF CACHE BOOL "" FORCE)
set(BUILD_PYBULLET OFF CACHE BOOL "" FORCE)
set(USE_GRAPHICAL_BENCHMARK OFF CACHE BOOL "" FORCE)
set(ENABLE_VHACD OFF CACHE BOOL "" FORCE)
set(INSTALL_CMAKE_FILES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

add_subdirectory(external/bullet3 EXCLUDE_FROM_ALL)

set_target_properties(BulletCollision LinearMath PROPERTIES SYSTEM TRUE)

# --- Executable -----------------------------------------------------------

target_compile_features(decade PRIVATE cxx_std_26)
set_target_properties(decade PROPERTIES AUTOMOC ON)

target_include_directories(decade PRIVATE src)

target_include_directories(decade SYSTEM PRIVATE
	${EPOXY_INCLUDE_DIRS}
	${ICU_INCLUDE_DIRS}
	external/csv2/include
	external/tinycolormap/include
	external/bullet3/src
)

target_link_libraries(decade PRIVATE
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

target_compile_definitions(decade PRIVATE GLM_ENABLE_EXPERIMENTAL)

# --- Compiler -----------------------------------------------------------

include(CMakePrintHelpers)
cmake_print_variables(
	CMAKE_CXX_COMPILER_ID
	CMAKE_CXX_COMPILER_VERSION
	CMAKE_CXX_STANDARD
	CMAKE_CXX_STANDARD_REQUIRED
	CMAKE_CXX_EXTENSIONS
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_compile_options(decade PRIVATE
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

	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		target_compile_options(decade PRIVATE
			-Wlogical-op
			-Wduplicated-cond
			-Wduplicated-branches
			-Wuseless-cast
		)
	endif()

	if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		target_compile_options(decade PRIVATE
			-Wunreachable-code
			-Wextra-semi
		)
	endif()
endif()

# --- IPOSupported -----------------------------------------------------------

include(CheckIPOSupported)
check_ipo_supported(RESULT decade_ipo_supported)
if(decade_ipo_supported)
	set_target_properties(decade PROPERTIES
		INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
		INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
endif()

option(DECADE_BUILD_TESTS "Build the unit tests (tests/ mirrors src/)" ON)
if(DECADE_BUILD_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif()

# --- Gates --------------------------------------------------------------------

add_custom_target(sanitize-address
	COMMAND ${CMAKE_COMMAND} -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_BINARY_DIR}/san-address -G ${CMAKE_GENERATOR}
		-DCMAKE_BUILD_TYPE=RelWithDebInfo
		"-DCMAKE_CXX_FLAGS=-D_GLIBCXX_ASSERTIONS -fsanitize=address,leak,undefined,float-cast-overflow,float-divide-by-zero -fno-sanitize-recover=undefined -fno-omit-frame-pointer"
	COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/san-address
	COMMAND ${CMAKE_COMMAND} -E env UBSAN_OPTIONS=print_stacktrace=1
		${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR}/san-address --output-on-failure
	COMMENT "Sanitizer gate: address, leak, undefined, float checks, container assertions"
	VERBATIM USES_TERMINAL)


if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

	target_compile_options(decade PRIVATE -Wno-c23-extensions)

	# The driver has to match the compiler that configured this directory.
	# A distribution carries several LLVM majors side by side, and the
	# unversioned name points at whichever one it calls default — on Ubuntu
	# 26.04 that is 21 while the developer machines run 22. The gate then
	# reports findings nobody can reproduce: clang-tidy 21 asks for a `const`
	# pointee on a pointer the function returns non-const, which does not
	# compile (measured 2026-08-10 on `SceneTreePanel::FindItemAt`).
	string(REGEX MATCH "^[0-9]+" DECADE_CLANG_MAJOR ${CMAKE_CXX_COMPILER_VERSION})
	find_program(DECADE_RUN_CLANG_TIDY
		NAMES run-clang-tidy-${DECADE_CLANG_MAJOR} run-clang-tidy
		REQUIRED)

	add_custom_target(clang-tidy
		COMMAND ${DECADE_RUN_CLANG_TIDY} -p ${CMAKE_BINARY_DIR}
			-warnings-as-errors=*
			${CMAKE_CURRENT_SOURCE_DIR}/src/
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMENT "clang-tidy gate"
		VERBATIM USES_TERMINAL)

	add_custom_target(sanitize-memory
		COMMAND ${CMAKE_COMMAND} -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_BINARY_DIR}/san-memory -G ${CMAKE_GENERATOR}
			-DCMAKE_BUILD_TYPE=RelWithDebInfo
			"-DCMAKE_CXX_FLAGS=-fsanitize=memory -fno-omit-frame-pointer"
		COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/san-memory
		COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR}/san-memory --output-on-failure
		COMMENT "Sanitizer gate -fsanitize=memory"
		VERBATIM USES_TERMINAL)
endif()
