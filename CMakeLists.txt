cmake_minimum_required(VERSION 3.16)

project(decade VERSION 0.0.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Every header pairs with a translation unit of the same name (AGENTS.md,
# "Components: a header and its translation unit"). Both get globbed rather than
# hand-listed, and CONFIGURE_DEPENDS makes CMake re-glob when a file appears —
# with the sources compiled now, a missing entry breaks the link instead of
# vanishing from the IDE tree in silence.
#
# The components sit in a library so the executable and the test binary share
# one compilation of them. main.cpp stays outside it: a test binary carries an
# entry point of its own. Every dependency below hangs on the library as PUBLIC
# and reaches the executable through the link — one place names each flag.
add_library(decade_components STATIC)
add_executable(decade src/application/main.cpp)

file(GLOB_RECURSE decade_headers CONFIGURE_DEPENDS src/*.hpp)
file(GLOB_RECURSE decade_sources CONFIGURE_DEPENDS src/*.cpp)
list(REMOVE_ITEM decade_sources ${CMAKE_CURRENT_SOURCE_DIR}/src/application/main.cpp)
target_sources(decade_components PRIVATE ${decade_sources} ${decade_headers})

target_compile_features(decade_components PUBLIC cxx_std_26)
target_link_libraries(decade PRIVATE decade_components)

# Cutting the program into components moved the inlining decision across unit
# borders, where the compiler alone no longer reaches it — every accessor that
# used to inline out of a header would turn into a call. LTO hands that decision
# to the linker. Debug builds stay out: nothing inlines there anyway, and the
# link time would only lengthen the edit cycle.
include(CheckIPOSupported)
check_ipo_supported(RESULT decade_ipo_supported)
if(decade_ipo_supported)
	set_target_properties(decade decade_components PROPERTIES
		INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
		INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
endif()

# The warning set belongs to the units we compile ourselves, never to whoever
# links against them: the tests build under a milder set of their own
# (tests/CMakeLists.txt), and a PUBLIC option would push -Werror onto them
# through the link. So one list, applied to both of our targets.
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

# epoxy claims the include guards __gl_h_ and __glext_h_ and refuses to compile
# once GL/gl.h got there first (epoxy/gl.h:38, an #error). Qt's qopengl.h — which
# QOpenGLWidget pulls in — includes exactly those, so epoxy has to be first in
# every translation unit. Source order cannot guarantee that: clang-format sorts
# <QtOpenGLWidgets/...> ahead of <epoxy/gl.h>, and Google style regroups include
# blocks. The force-include settles it in one place for the whole target; with
# the guards taken, Qt's GL headers collapse to nothing and every gl* call
# resolves through epoxy's dispatch. PRIVATE for the same reason as the warning
# set: it belongs to the units we compile, not to the test binary that links them.
target_compile_options(decade_components PRIVATE -include epoxy/gl.h)
target_compile_options(decade PRIVATE -include epoxy/gl.h)

# ICU: Unicode text handling (UTF-8, NFC, grapheme segmentation) in font.hpp plus
# the date backend. icu-uc = Unicode and common, icu-i18n = BreakIterator and
# calendar. Not every distribution ships a CMake config, so through pkg-config
# like epoxy.
pkg_check_modules(ICU REQUIRED icu-uc icu-i18n)
target_include_directories(decade_components SYSTEM PUBLIC ${ICU_INCLUDE_DIRS})
target_link_libraries(decade_components PUBLIC ${ICU_LIBRARIES})

# Qt: the widget toolkit and the GL surface. PrintSupport carries
# QPageSetupDialog, the paper-size and margin dialogue behind the page panel.
#
# AUTOMOC is on: with a translation unit per header (AGENTS.md, "Components: a
# header and its translation unit") the unit moc emits costs nothing the design
# forbids, so a presentation class may carry Q_OBJECT. Reaching for it still wants
# a reason — the application's own signals run over sigslot, and QObject::connect
# with a functor subscribes to a Qt widget's signal without any of it.
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

# clang-tidy reads compile_commands.json, and Qt6::Platform writes a GCC-only
# codegen flag into it: -mno-direct-extern-access, chosen by a generator
# expression on the compiler id. Clang rejects it as an unknown *argument*, which
# is a driver error — no -Wno-… reaches it, unlike the GCC warning flags below.
# So the gate analyses a filtered copy of the database. Taking the flag out of
# the build instead would be the wrong trade: it stops GCC from assuming direct
# access to Qt's exported data symbols, and Qt sets it deliberately.
set(DECADE_TIDY_DB_DIR ${CMAKE_BINARY_DIR}/tidy)
add_custom_target(clang-tidy-db
	COMMAND ${CMAKE_COMMAND} -E make_directory ${DECADE_TIDY_DB_DIR}
	COMMAND sh -c "sed 's/-mno-direct-extern-access//g' '${CMAKE_BINARY_DIR}/compile_commands.json' > '${DECADE_TIDY_DB_DIR}/compile_commands.json'"
	COMMENT "compile_commands.json for clang-tidy, without the GCC-only flags"
	VERBATIM)

# clang-tidy over every translation unit under src/, each one carrying its own
# headers in through HeaderFilterRegex. run-clang-tidy fans the units out over
# the cores; a plain clang-tidy call would walk the hundred-odd of them in a row.
# The positional argument is a regex against the paths in the database — the
# source prefix keeps the external/ submodules and the generated moc unit out.
# The -Wno-* extra args defuse GCC warning flags out of compile_commands.json;
# the epoxy force-include rides along in there by itself, because it stands as a
# target compile option.
#
# -Wno-c23-extensions is of the same kind and is NOT a suppressed finding: it
# says nothing about our code, only that Clang 22 has not yet implemented
# #embed as the C++26 feature it is and still grades it as an extension. GCC —
# what builds here and in CI — compiles it as standard under -std=c++26 with
# -Wpedantic -Werror. Drop this the day Clang implements it.
add_custom_target(clang-tidy
	COMMAND run-clang-tidy -p ${DECADE_TIDY_DB_DIR}
		-warnings-as-errors=*
		-extra-arg=-Wno-error -extra-arg=-Wno-unknown-warning-option
		-extra-arg=-Wno-c23-extensions
		${CMAKE_CURRENT_SOURCE_DIR}/src/
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	COMMENT "clang-tidy gate (every finding = an error) over every unit in src/"
	VERBATIM USES_TERMINAL)
add_dependencies(clang-tidy clang-tidy-db)

# Sanitizers demand other compile flags than the normal build, so every gate
# configures a build folder of its own, rebuilds everything in it and runs the
# test suite — a run finds errors, a build alone does not. Should a sanitizer
# fire, the process ends non-zero and ctest breaks off. The flags sit in
# CMAKE_CXX_FLAGS, so they hit every target in the folder, Bullet included.
# UBSan would otherwise merely report and carry on — without
# -fno-sanitize-recover the gate would stay green; -fno-omit-frame-pointer saves
# the stack traces.
# https://gcc.gnu.org/onlinedocs/gcc-9.2.0/gcc/Instrumentation-Options.html
# LeakSanitizer already sits inside AddressSanitizer on Linux; named here anyway.
# _GLIBCXX_ASSERTIONS is no sanitizer but closes a hole they leave: reading
# past the size of a container that still has capacity stays inside the
# allocation, so AddressSanitizer sees nothing (v.front() on an empty vector
# after reserve() returns garbage and runs on).
# float-cast-overflow stands beside `undefined` because GCC does not fold it
# in: a float too large for the integer it gets cast to is undefined, and
# without this flag the gate reads that cast as sound (verified against GCC 16
# on 2026-08-06). That is the shape of #28.
add_custom_target(sanitize-address
	COMMAND ${CMAKE_COMMAND} -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_BINARY_DIR}/san-address -G ${CMAKE_GENERATOR}
		-DCMAKE_BUILD_TYPE=RelWithDebInfo
		"-DCMAKE_CXX_FLAGS=-D_GLIBCXX_ASSERTIONS -fsanitize=address,leak,undefined,float-cast-overflow,float-divide-by-zero -fno-sanitize-recover=undefined -fno-omit-frame-pointer"
	COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/san-address
	COMMAND ${CMAKE_COMMAND} -E env UBSAN_OPTIONS=print_stacktrace=1
		${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR}/san-address --output-on-failure
	COMMENT "Sanitizer gate: address, leak, undefined, float checks, container assertions"
	VERBATIM USES_TERMINAL)

# MemorySanitizer excludes AddressSanitizer (the compiler rejects the
# combination) and clang alone knows it — hence a second gate. It demands that
# ALL dependencies be instrumented; with the system libstdc++ and gtest it reports
# false alarms out of foreign code. A diagnostic tool, no CI gate — see
# operations.md.
add_custom_target(sanitize-memory
	COMMAND ${CMAKE_COMMAND} -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_BINARY_DIR}/san-memory -G ${CMAKE_GENERATOR}
		-DCMAKE_BUILD_TYPE=RelWithDebInfo
		-DCMAKE_CXX_COMPILER=clang++
		"-DCMAKE_CXX_FLAGS=-fsanitize=memory -fno-omit-frame-pointer"
	COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/san-memory
	COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR}/san-memory --output-on-failure
	COMMENT "Sanitizer gate -fsanitize=memory (build plus test run, clang)"
	VERBATIM USES_TERMINAL)
