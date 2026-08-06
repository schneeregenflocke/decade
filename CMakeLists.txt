cmake_minimum_required(VERSION 3.15)

project(decade VERSION 0.0.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(decade)
target_compile_features(decade PUBLIC cxx_std_23)

# The single translation unit; everything else is a header (see AGENTS.md). They
# get collected for the IDE listing alone, not compiled — hence a glob instead of
# a hand-kept list: a forgotten line breaks nothing, it merely goes missing from
# the tree in silence.
file(GLOB_RECURSE decade_headers CONFIGURE_DEPENDS src/*.hpp)
target_sources(decade PRIVATE src/application/main.cpp ${decade_headers})

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
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
	else()
		target_compile_options(decade PRIVATE
			-Wunreachable-code
			-Wextra-semi
		)
	endif()
endif()

# --- System dependencies ------------------------------------------------------
# Foreign headers everywhere as SYSTEM: their warnings and clang-tidy findings
# must not break the -Werror gate.

find_package(Boost CONFIG REQUIRED COMPONENTS serialization wserialization)
target_link_libraries(decade PRIVATE Boost::serialization Boost::wserialization)

find_package(PkgConfig REQUIRED)

# GL pulls libepoxy in itself at runtime; there is nothing to link.
pkg_check_modules(EPOXY REQUIRED epoxy)
target_include_directories(decade SYSTEM PRIVATE ${EPOXY_INCLUDE_DIRS})
target_link_libraries(decade PRIVATE ${EPOXY_LIBRARIES})

# ICU: Unicode text handling (UTF-8, NFC, grapheme segmentation) in font.hpp plus
# the date backend. icu-uc = Unicode and common, icu-i18n = BreakIterator and
# calendar. Not every distribution ships a CMake config, so through pkg-config
# like epoxy.
pkg_check_modules(ICU REQUIRED icu-uc icu-i18n)
target_include_directories(decade SYSTEM PRIVATE ${ICU_INCLUDE_DIRS})
target_link_libraries(decade PRIVATE ${ICU_LIBRARIES})

find_package(wxWidgets CONFIG COMPONENTS propgrid gl core base)
if(NOT wxWidgets_FOUND)
	# Debian and Ubuntu build wx with autotools and ship no CMake config: module
	# mode through wx-config. In CONFIG mode the imported targets handle includes
	# and flags themselves.
	find_package(wxWidgets REQUIRED COMPONENTS propgrid gl core base)
	target_include_directories(decade SYSTEM PRIVATE ${wxWidgets_INCLUDE_DIRS})
	target_compile_definitions(decade PRIVATE ${wxWidgets_DEFINITIONS})
	separate_arguments(wxWidgets_cxx_flags UNIX_COMMAND "${wxWidgets_CXX_FLAGS}")
	target_compile_options(decade PRIVATE ${wxWidgets_cxx_flags})
endif()
target_link_libraries(decade PRIVATE ${wxWidgets_LIBRARIES})

find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(Fontconfig REQUIRED)
find_package(Freetype REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Microsoft.GSL CONFIG REQUIRED)
target_link_libraries(decade PRIVATE
	ZLIB::ZLIB
	PNG::PNG
	Fontconfig::Fontconfig
	Freetype::Freetype
	glm::glm
	Microsoft.GSL::GSL
)
target_compile_definitions(decade PRIVATE GLM_ENABLE_EXPERIMENTAL)

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
target_include_directories(decade SYSTEM PRIVATE
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
target_link_libraries(decade PRIVATE BulletCollision LinearMath)
set_target_properties(BulletCollision LinearMath PROPERTIES SYSTEM TRUE)

add_subdirectory(external/embed-resource)
target_include_directories(decade SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/embed-resource)
# embed-resource runs during the build. Instrumented, a finding in it would break
# the build instead of checking the program — so exempt it from the sanitizer.
target_compile_options(embed-resource PRIVATE -fno-sanitize=all)
target_link_options(embed-resource PRIVATE -fno-sanitize=all)

# --- Embedded resources (the name after the file = the access name in code) ---

embed_resources(decade_resources
	shaders/font_fragment_shader.glsl shader
	shaders/font_vertex_shader.glsl shader
	shaders/lambert_fragment_shader.glsl shader
	shaders/lambert_vertex_shader.glsl shader
	shaders/simple_fragment_shader.glsl shader
	shaders/simple_vertex_shader.glsl shader
	shaders/rectangles_fragment_shader.glsl shader
	shaders/rectangles_vertex_shader.glsl shader
	LICENSE.txt decade
	external/embed-resource/LICENSE.txt embed_resource_LICENSE
	external/sigslot/LICENSE sigslot_LICENSE
	external/csv2/LICENSE csv2_copyright
	external/csv2/LICENSE.mio csv2mio_LICENSE
	external/tinycolormap/LICENSE tinycolormap_LICENSE
	external/bullet3/LICENSE.txt bullet_LICENSE
)
target_sources(decade PRIVATE ${decade_resources})

option(DECADE_BUILD_TESTS "Build the unit tests (tests/ mirrors src/)" ON)
if(DECADE_BUILD_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif()

# --- Gates --------------------------------------------------------------------
# Deliberately no part of the standard build, so normal compiles stay fast; call
# them explicitly or in CI. The commands sit in operations.md.

# clang-tidy on the single translation unit — it covers every src/ header through
# HeaderFilterRegex. --extra-arg: -include type_traits works around an include bug
# in wx/meta/convertible.h, and the -Wno-* defuse GCC flags out of
# compile_commands.json.
add_custom_target(clang-tidy
	COMMAND clang-tidy -p ${CMAKE_BINARY_DIR}
		--warnings-as-errors=*
		--extra-arg=-include --extra-arg=type_traits
		--extra-arg=-Wno-error --extra-arg=-Wno-unknown-warning-option
		src/application/main.cpp
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	COMMENT "clang-tidy gate (every finding = an error) on src/application/main.cpp"
	VERBATIM USES_TERMINAL)

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
# float-cast-overflow stands beside `undefined` because GCC does not fold it
# in: a float too large for the integer it gets cast to is undefined, and
# without this flag the gate reads that cast as sound (verified against GCC 16
# on 2026-08-06). That is the shape of #28.
add_custom_target(sanitize-address
	COMMAND ${CMAKE_COMMAND} -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_BINARY_DIR}/san-address -G ${CMAKE_GENERATOR}
		-DCMAKE_BUILD_TYPE=RelWithDebInfo
		"-DCMAKE_CXX_FLAGS=-fsanitize=address,leak,undefined,float-cast-overflow,float-divide-by-zero -fno-sanitize-recover=undefined -fno-omit-frame-pointer"
	COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/san-address
	COMMAND ${CMAKE_COMMAND} -E env UBSAN_OPTIONS=print_stacktrace=1
		${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR}/san-address --output-on-failure
	COMMENT "Sanitizer gate -fsanitize=address,leak,undefined,float-cast-overflow,float-divide-by-zero (build plus test run)"
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
