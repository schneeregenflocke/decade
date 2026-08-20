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

# --- SBOM ---------------------------------------------------------------------

# The licence texts the about dialogue shows sit in `licenses/`; this is the
# machine-readable half of the same knowledge. Both are fed from the build
# rather than from a hand-kept list: the versions below are what `find_package`
# and `pkg_check_modules` just resolved, so the SBOM cannot drift from what the
# linker actually took. The submodules answer with their pinned commit instead
# of a version, which is the only thing that identifies them exactly.
#
# GoogleTest is deliberately absent: it links into `decade_tests` alone and
# never reaches the artefact this document describes.

include(GNUInstallDirs)
install(TARGETS decade RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY licenses DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/decade)

list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_SOURCE_DIR}/external/cmake-sbom/cmake")
include(sbom)

find_package(Git QUIET REQUIRED)
function(decade_submodule_commit relative_path output_variable)
	execute_process(
		COMMAND ${GIT_EXECUTABLE} -C ${CMAKE_SOURCE_DIR}/${relative_path} rev-parse HEAD
		OUTPUT_VARIABLE commit
		OUTPUT_STRIP_TRAILING_WHITESPACE
		COMMAND_ERROR_IS_FATAL ANY)
	set(${output_variable} ${commit} PARENT_SCOPE)
endfunction()

decade_submodule_commit(external/bullet3 DECADE_BULLET_COMMIT)
decade_submodule_commit(external/csv2 DECADE_CSV2_COMMIT)
decade_submodule_commit(external/tinycolormap DECADE_TINYCOLORMAP_COMMIT)

sbom_generate(
	OUTPUT ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATAROOTDIR}/decade/decade.spdx
	LICENSE MIT
	SUPPLIER "Marco Peyer"
	SUPPLIER_URL https://github.com/schneeregenflocke
	DOWNLOAD_URL https://github.com/schneeregenflocke/decade
)

sbom_add(TARGET decade)

# Qt carries the only copyleft in the tree. LGPL-3.0-only is a choice out of the
# licences Qt offers, not a fact about the package: the GPL-2.0 option would
# pull FreeType onto its GPL branch and take the whole binary with it.
sbom_add(PACKAGE Qt6 VERSION ${Qt6_VERSION} LICENSE LGPL-3.0-only
	SUPPLIER "Organization: The Qt Company" DOWNLOAD_LOCATION https://download.qt.io/)
sbom_add(PACKAGE boost VERSION ${Boost_VERSION} LICENSE BSL-1.0
	SUPPLIER "Organization: Boost" DOWNLOAD_LOCATION https://www.boost.org/)
sbom_add(PACKAGE icu VERSION ${ICU_icu-uc_VERSION}
	LICENSE "Unicode-3.0 AND BSD-2-Clause AND BSD-3-Clause AND NAIST-2003"
	SUPPLIER "Organization: Unicode, Inc." DOWNLOAD_LOCATION https://github.com/unicode-org/icu)
sbom_add(PACKAGE freetype VERSION ${FREETYPE_VERSION_STRING} LICENSE FTL
	SUPPLIER "Organization: The FreeType Project" DOWNLOAD_LOCATION https://freetype.org/)
sbom_add(PACKAGE fontconfig VERSION ${Fontconfig_VERSION}
	LICENSE "HPND AND Unicode-DFS-2016"
	SUPPLIER "Organization: freedesktop.org" DOWNLOAD_LOCATION https://www.freedesktop.org/wiki/Software/fontconfig/)
sbom_add(PACKAGE libpng VERSION ${PNG_VERSION_STRING} LICENSE libpng-2.0
	SUPPLIER "Organization: PNG Reference Library Authors" DOWNLOAD_LOCATION http://www.libpng.org/pub/png/libpng.html)
sbom_add(PACKAGE zlib VERSION ${ZLIB_VERSION_STRING} LICENSE Zlib
	SUPPLIER "Person: Jean-loup Gailly and Mark Adler" DOWNLOAD_LOCATION https://zlib.net/)
sbom_add(PACKAGE libepoxy VERSION ${EPOXY_VERSION} LICENSE MIT
	SUPPLIER "Organization: Intel Corporation" DOWNLOAD_LOCATION https://github.com/anholt/libepoxy)
sbom_add(PACKAGE glm VERSION ${glm_VERSION} LICENSE MIT
	SUPPLIER "Organization: G-Truc Creation" DOWNLOAD_LOCATION https://github.com/g-truc/glm)
sbom_add(PACKAGE Microsoft.GSL VERSION ${Microsoft.GSL_VERSION} LICENSE MIT
	SUPPLIER "Organization: Microsoft Corporation" DOWNLOAD_LOCATION https://github.com/microsoft/GSL)
sbom_add(PACKAGE bullet3 COMMIT ${DECADE_BULLET_COMMIT} LICENSE Zlib
	SUPPLIER "Person: Erwin Coumans" DOWNLOAD_LOCATION https://github.com/bulletphysics/bullet3)
sbom_add(PACKAGE csv2 COMMIT ${DECADE_CSV2_COMMIT} LICENSE MIT
	SUPPLIER "Person: Pranav Srinivas Kumar" DOWNLOAD_LOCATION https://github.com/p-ranav/csv2)
sbom_add(PACKAGE tinycolormap COMMIT ${DECADE_TINYCOLORMAP_COMMIT} LICENSE MIT
	SUPPLIER "Person: Yuki Koyama" DOWNLOAD_LOCATION https://github.com/yuki-koyama/tinycolormap)

# NO_VERIFY, because cmake-sbom's check needs the Python packages spdx-tools
# and ntia-conformance-checker, and it turns a missing package into a failing
# `cmake --install`. Verifying stays a gate you run on purpose — the command
# stands in operations.md.
sbom_finalize(NO_VERIFY)

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
