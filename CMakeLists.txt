cmake_minimum_required(VERSION 3.16)

project(decade VERSION 0.0.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# One compiler per build directory: configure build/ with GCC or with clang, and
# the targets belonging to that compiler appear. Everything below that differs
# between them stands under DECADE_GCC or DECADE_CLANG — what holds for both
# carries no guard at all.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(DECADE_GCC ON)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set(DECADE_CLANG ON)
endif()

# CONFIGURE_DEPENDS re-globs when a file appears: with the sources compiled, a
# missing entry breaks the link instead of vanishing from the IDE tree in silence.
file(GLOB_RECURSE decade_headers CONFIGURE_DEPENDS src/*.hpp)
file(GLOB_RECURSE decade_sources CONFIGURE_DEPENDS src/*.cpp)

add_executable(decade ${decade_sources} ${decade_headers})

# --- Dependencies -------------------------------------------------------------
# An INTERFACE target compiles nothing and produces no archive; it carries what
# both binaries need in order to compile our sources, so the test binary
# inherits that instead of repeating it. Foreign headers arrive SYSTEM, so their
# warnings and clang-tidy findings cannot break the -Werror gate.

add_library(decade_dependencies INTERFACE)
target_compile_features(decade_dependencies INTERFACE cxx_std_26)
target_include_directories(decade_dependencies INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# CONFIG only where the find module is in the way: CMake drops FindBoost from
# 3.30 on (CMP0167), because upstream ships BoostConfig.cmake. Everywhere else
# the basic signature falls back to the config package by itself.
find_package(Boost CONFIG REQUIRED COMPONENTS serialization wserialization)
find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(Fontconfig REQUIRED)
find_package(Freetype REQUIRED)
find_package(glm REQUIRED)
find_package(Microsoft.GSL REQUIRED)

# PrintSupport carries QPageSetupDialog, the dialogue behind the page panel.
# AUTOMOC is on, so a presentation class may carry Q_OBJECT — which still wants
# a reason of its own; AGENTS.md, "Components: a header and its translation unit".
find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets PrintSupport)

find_package(PkgConfig REQUIRED)
# GL pulls libepoxy in itself at runtime; there is nothing to link.
pkg_check_modules(EPOXY REQUIRED epoxy)
# icu-uc = Unicode and common, icu-i18n = BreakIterator and calendar. Through
# pkg-config like epoxy, because not every distribution ships a CMake config.
pkg_check_modules(ICU REQUIRED icu-uc icu-i18n)

target_link_libraries(decade_dependencies INTERFACE
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
	${EPOXY_LIBRARIES}
	${ICU_LIBRARIES}
)
target_include_directories(decade_dependencies SYSTEM INTERFACE
	${EPOXY_INCLUDE_DIRS}
	${ICU_INCLUDE_DIRS}
)
target_compile_definitions(decade_dependencies INTERFACE GLM_ENABLE_EXPERIMENTAL)

# epoxy claims __gl_h_ and __glext_h_ and #errors out if Qt's qopengl.h got
# there first. Source order cannot settle it — clang-format sorts the includes —
# so the force-include does, for everyone compiling our sources. AGENTS.md,
# "OpenGL under Qt".
target_compile_options(decade_dependencies INTERFACE -include epoxy/gl.h)

# --- Submodules out of external/ ----------------------------------------------

function(decade_require_submodule name probe_file)
	if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/${name}/${probe_file}")
		message(FATAL_ERROR "${name} is missing. Fetch the submodule: git submodule update --init --recursive")
	endif()
endfunction()

decade_require_submodule(csv2 include/csv2/reader.hpp)
decade_require_submodule(sigslot include/sigslot/signal.hpp)
decade_require_submodule(tinycolormap include/tinycolormap.hpp)
decade_require_submodule(bullet3 src/btBulletCollisionCommon.h)

target_include_directories(decade_dependencies SYSTEM INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}/external/csv2/include
	${CMAKE_CURRENT_SOURCE_DIR}/external/tinycolormap/include
	${CMAKE_CURRENT_SOURCE_DIR}/external/sigslot/include
	${CMAKE_CURRENT_SOURCE_DIR}/external/bullet3/src
)

# Bullet from source, everything superfluous off. Picking needs the Bullet 2
# collision world alone (BulletCollision plus LinearMath); EXCLUDE_FROM_ALL keeps
# the rest out of the standard build.
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
# Bullet 3.25 still declares cmake_minimum_required < 3.5, which CMake 4.x rejects.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
add_subdirectory(external/bullet3 EXCLUDE_FROM_ALL)
set_target_properties(BulletCollision LinearMath PROPERTIES SYSTEM TRUE)
target_link_libraries(decade_dependencies INTERFACE BulletCollision LinearMath)

# Resources travel through #embed (src/common/embedded_resources.hpp): the
# preprocessor reads the files, so there is no generator target and no submodule.

# --- Warnings -----------------------------------------------------------------
# PRIVATE on the target, never on decade_dependencies: through the interface it
# would push -Werror onto the tests, which build under a milder set of their own
# (tests/CMakeLists.txt).

# Both compilers
set(decade_warnings
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

if(DECADE_GCC)
	list(APPEND decade_warnings
		-Wlogical-op
		-Wduplicated-cond
		-Wduplicated-branches
		-Wuseless-cast
	)
endif()

if(DECADE_CLANG)
	list(APPEND decade_warnings
		-Wunreachable-code
		-Wextra-semi
	)
	# Clang 22 has not implemented #embed as the C++26 feature it is and grades
	# it as an extension, which -Werror ends on; GCC compiles the same line as
	# standard. On the interface, because the tests compile that header too.
	target_compile_options(decade_dependencies INTERFACE -Wno-c23-extensions)
endif()

target_link_libraries(decade PRIVATE decade_dependencies)
target_compile_options(decade PRIVATE ${decade_warnings})
set_target_properties(decade PROPERTIES AUTOMOC ON)

# Components moved the inlining decision across unit borders, out of the
# compiler's reach; LTO hands it to the linker. Debug stays out — nothing
# inlines there anyway, and the link time would only lengthen the edit cycle.
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
# Deliberately no part of the standard build, so normal compiles stay fast; call
# them explicitly or in CI. The commands sit in operations.md.

# Both compilers: the sanitizers instrument with whoever configured the tree. A
# folder of its own, because the flags differ from the build, and a test run
# rather than a build, because a run is what finds errors. Flags: operations.md.
add_custom_target(sanitize-address
	COMMAND ${CMAKE_COMMAND} -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_BINARY_DIR}/san-address -G ${CMAKE_GENERATOR}
		-DCMAKE_BUILD_TYPE=RelWithDebInfo
		"-DCMAKE_CXX_FLAGS=-D_GLIBCXX_ASSERTIONS -fsanitize=address,leak,undefined,float-cast-overflow,float-divide-by-zero -fno-sanitize-recover=undefined -fno-omit-frame-pointer"
	COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/san-address
	COMMAND ${CMAKE_COMMAND} -E env UBSAN_OPTIONS=print_stacktrace=1
		${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR}/san-address --output-on-failure
	COMMENT "Sanitizer gate: address, leak, undefined, float checks, container assertions"
	VERBATIM USES_TERMINAL)

if(DECADE_CLANG)
	# clang-tidy parses with clang's frontend and dies on a GCC database, where
	# Qt6::Platform leaves -mno-direct-extern-access behind. The positional
	# argument is a regex: the source prefix keeps external/ and the moc unit out.
	add_custom_target(clang-tidy
		COMMAND run-clang-tidy -p ${CMAKE_BINARY_DIR}
			-warnings-as-errors=*
			${CMAKE_CURRENT_SOURCE_DIR}/src/
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMENT "clang-tidy gate (every finding = an error) over every unit in src/"
		VERBATIM USES_TERMINAL)

	# MemorySanitizer excludes AddressSanitizer and clang alone knows it. A
	# diagnostic tool, no gate: it wants every dependency instrumented, and the
	# system libstdc++ and gtest are not (#51).
	add_custom_target(sanitize-memory
		COMMAND ${CMAKE_COMMAND} -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_BINARY_DIR}/san-memory -G ${CMAKE_GENERATOR}
			-DCMAKE_BUILD_TYPE=RelWithDebInfo
			"-DCMAKE_CXX_FLAGS=-fsanitize=memory -fno-omit-frame-pointer"
		COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/san-memory
		COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR}/san-memory --output-on-failure
		COMMENT "Sanitizer gate -fsanitize=memory (build plus test run, clang)"
		VERBATIM USES_TERMINAL)
endif()
