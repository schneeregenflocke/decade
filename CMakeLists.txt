cmake_minimum_required(VERSION 3.16)

project(decade VERSION 0.0.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# A library, so executable and test binary share one compilation of the
# components; main.cpp stays out because a test binary brings its own entry
# point. Dependencies hang on the library PUBLIC and reach the executable.
add_library(decade_components STATIC)
add_executable(decade src/application/main.cpp)

# CONFIGURE_DEPENDS re-globs when a file appears: with the sources compiled, a
# missing entry breaks the link instead of vanishing from the IDE tree in silence.
file(GLOB_RECURSE decade_headers CONFIGURE_DEPENDS src/*.hpp)
file(GLOB_RECURSE decade_sources CONFIGURE_DEPENDS src/*.cpp)
list(REMOVE_ITEM decade_sources ${CMAKE_CURRENT_SOURCE_DIR}/src/application/main.cpp)
target_sources(decade_components PRIVATE ${decade_sources} ${decade_headers})

target_compile_features(decade_components PUBLIC cxx_std_26)
target_link_libraries(decade PRIVATE decade_components)

# Components moved the inlining decision across unit borders, out of the
# compiler's reach; LTO hands it to the linker. Debug stays out — nothing
# inlines there anyway, and the link time would only lengthen the edit cycle.
include(CheckIPOSupported)
check_ipo_supported(RESULT decade_ipo_supported)
if(decade_ipo_supported)
	set_target_properties(decade decade_components PROPERTIES
		INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
		INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
endif()

# PRIVATE, never PUBLIC: the set belongs to the units we compile, and through
# the link it would push -Werror onto the tests, which build under a milder set
# of their own (tests/CMakeLists.txt).
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
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
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		list(APPEND decade_warnings
			-Wlogical-op
			-Wduplicated-cond
			-Wduplicated-branches
			-Wuseless-cast
		)
	else()
		list(APPEND decade_warnings
			-Wunreachable-code
			-Wextra-semi
		)
	endif()
	target_compile_options(decade_components PRIVATE ${decade_warnings})
	target_compile_options(decade PRIVATE ${decade_warnings})
endif()

# Clang 22 has not implemented #embed as the C++26 feature it is and grades it
# as an extension; GCC compiles the same line as standard. PUBLIC, because the
# header carrying it reaches everyone who links the components — the tests too.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	target_compile_options(decade_components PUBLIC -Wno-c23-extensions)
endif()

# --- System dependencies ------------------------------------------------------
# Foreign headers everywhere as SYSTEM: their warnings and clang-tidy findings
# must not break the -Werror gate.

find_package(Boost CONFIG REQUIRED COMPONENTS serialization wserialization)
target_link_libraries(decade_components PUBLIC Boost::serialization Boost::wserialization)

find_package(PkgConfig REQUIRED)

# GL pulls libepoxy in itself at runtime; there is nothing to link.
pkg_check_modules(EPOXY REQUIRED epoxy)
target_include_directories(decade_components SYSTEM PUBLIC ${EPOXY_INCLUDE_DIRS})
target_link_libraries(decade_components PUBLIC ${EPOXY_LIBRARIES})

# epoxy claims __gl_h_ and __glext_h_ and #errors out if Qt's qopengl.h got
# there first. Source order cannot settle it — clang-format sorts the includes —
# so the force-include does, once per target. AGENTS.md, "OpenGL under Qt".
target_compile_options(decade_components PRIVATE -include epoxy/gl.h)
target_compile_options(decade PRIVATE -include epoxy/gl.h)

# icu-uc = Unicode and common, icu-i18n = BreakIterator and calendar. Through
# pkg-config like epoxy, because not every distribution ships a CMake config.
pkg_check_modules(ICU REQUIRED icu-uc icu-i18n)
target_include_directories(decade_components SYSTEM PUBLIC ${ICU_INCLUDE_DIRS})
target_link_libraries(decade_components PUBLIC ${ICU_LIBRARIES})

# PrintSupport carries QPageSetupDialog, the dialogue behind the page panel.
# AUTOMOC is on, so a presentation class may carry Q_OBJECT — which still wants
# a reason of its own; AGENTS.md, "Components: a header and its translation unit".
find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets PrintSupport)
set_target_properties(decade decade_components PROPERTIES AUTOMOC ON)
target_link_libraries(decade_components PUBLIC
	Qt6::Widgets
	Qt6::OpenGLWidgets
	Qt6::PrintSupport
)

find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(Fontconfig REQUIRED)
find_package(Freetype REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Microsoft.GSL CONFIG REQUIRED)
target_link_libraries(decade_components PUBLIC
	ZLIB::ZLIB
	PNG::PNG
	Fontconfig::Fontconfig
	Freetype::Freetype
	glm::glm
	Microsoft.GSL::GSL
)
target_compile_definitions(decade_components PUBLIC GLM_ENABLE_EXPERIMENTAL)

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

set(CSV2_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/csv2/include")
set(TINYCOLORMAP_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/tinycolormap/include")
target_include_directories(decade_components SYSTEM PUBLIC
	${CSV2_INCLUDE_DIR}
	${TINYCOLORMAP_INCLUDE_DIR}
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
target_link_libraries(decade_components PUBLIC BulletCollision LinearMath)
set_target_properties(BulletCollision LinearMath PROPERTIES SYSTEM TRUE)

# Resources travel through #embed (src/common/embedded_resources.hpp): the
# preprocessor reads the files, so there is no generator target and no submodule.

option(DECADE_BUILD_TESTS "Build the unit tests (tests/ mirrors src/)" ON)
if(DECADE_BUILD_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif()

# --- Gates --------------------------------------------------------------------
# Deliberately no part of the standard build, so normal compiles stay fast; call
# them explicitly or in CI. The commands sit in operations.md.

# clang-tidy parses with clang's frontend and dies on a GCC database, where
# Qt6::Platform leaves -mno-direct-extern-access behind. So the gate exists in a
# clang tree alone — operations.md configures one.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	# The positional argument is a regex against the database paths: the source
	# prefix keeps the external/ submodules and the generated moc unit out.
	# run-clang-tidy fans the units over the cores, one clang-tidy walks them.
	add_custom_target(clang-tidy
		COMMAND run-clang-tidy -p ${CMAKE_BINARY_DIR}
			-warnings-as-errors=*
			${CMAKE_CURRENT_SOURCE_DIR}/src/
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMENT "clang-tidy gate (every finding = an error) over every unit in src/"
		VERBATIM USES_TERMINAL)
endif()

# A folder of its own, because sanitizers demand other flags, and a test run
# rather than a build, because a run is what finds errors. CMAKE_CXX_FLAGS to
# reach every target in it, Bullet included. Why each flag: operations.md.
add_custom_target(sanitize-address
	COMMAND ${CMAKE_COMMAND} -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_BINARY_DIR}/san-address -G ${CMAKE_GENERATOR}
		-DCMAKE_BUILD_TYPE=RelWithDebInfo
		"-DCMAKE_CXX_FLAGS=-D_GLIBCXX_ASSERTIONS -fsanitize=address,leak,undefined,float-cast-overflow,float-divide-by-zero -fno-sanitize-recover=undefined -fno-omit-frame-pointer"
	COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/san-address
	COMMAND ${CMAKE_COMMAND} -E env UBSAN_OPTIONS=print_stacktrace=1
		${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR}/san-address --output-on-failure
	COMMENT "Sanitizer gate: address, leak, undefined, float checks, container assertions"
	VERBATIM USES_TERMINAL)

# MemorySanitizer excludes AddressSanitizer and clang alone knows it, so it sits
# in the clang tree. A diagnostic tool, no gate: it wants every dependency
# instrumented, and the system libstdc++ and gtest are not (#51).
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	add_custom_target(sanitize-memory
		COMMAND ${CMAKE_COMMAND} -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_BINARY_DIR}/san-memory -G ${CMAKE_GENERATOR}
			-DCMAKE_BUILD_TYPE=RelWithDebInfo
			"-DCMAKE_CXX_FLAGS=-fsanitize=memory -fno-omit-frame-pointer"
		COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/san-memory
		COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR}/san-memory --output-on-failure
		COMMENT "Sanitizer gate -fsanitize=memory (build plus test run, clang)"
		VERBATIM USES_TERMINAL)
endif()
