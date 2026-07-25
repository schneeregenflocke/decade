cmake_minimum_required(VERSION 3.15)

project(decade VERSION 0.0.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(decade)
target_compile_features(decade PUBLIC cxx_std_23)

# Einzige Translation Unit; alles andere sind Header (siehe AGENTS.md). Sie werden
# nur fürs IDE-Listing eingesammelt, nicht kompiliert — darum Glob statt Handliste:
# eine vergessene Zeile bricht nichts, sie fehlt bloss still im Baum.
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

# --- Systemabhängigkeiten -----------------------------------------------------
# Fremde Header überall als SYSTEM: ihre Warnungen und clang-tidy-Befunde sollen
# das -Werror-Gate nicht brechen.

find_package(Boost CONFIG REQUIRED COMPONENTS serialization wserialization)
target_link_libraries(decade PRIVATE Boost::serialization Boost::wserialization)

find_package(PkgConfig REQUIRED)

# GL zieht libepoxy zur Laufzeit selbst nach; zu linken ist nichts.
pkg_check_modules(EPOXY REQUIRED epoxy)
target_include_directories(decade SYSTEM PRIVATE ${EPOXY_INCLUDE_DIRS})
target_link_libraries(decade PRIVATE ${EPOXY_LIBRARIES})

# ICU: Unicode-Textbehandlung (UTF-8, NFC, Graphem-Segmentierung) in font.hpp und
# das Datums-Backend. icu-uc = Unicode/common, icu-i18n = BreakIterator/Kalender.
# Nicht jede Distribution liefert eine CMake-Config, also via pkg-config wie epoxy.
pkg_check_modules(ICU REQUIRED icu-uc icu-i18n)
target_include_directories(decade SYSTEM PRIVATE ${ICU_INCLUDE_DIRS})
target_link_libraries(decade PRIVATE ${ICU_LIBRARIES})

find_package(wxWidgets CONFIG COMPONENTS propgrid gl core base)
if(NOT wxWidgets_FOUND)
	# Debian/Ubuntu bauen wx mit autotools und liefern keine CMake-Config:
	# Module-Mode via wx-config. Im CONFIG-Mode erledigen die imported Targets
	# Includes und Flags selbst.
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

# --- Untermodule aus external/ ------------------------------------------------

function(decade_require_submodule name probe_file)
	if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/${name}/${probe_file}")
		message(FATAL_ERROR "${name} fehlt. Untermodul holen: git submodule update --init --recursive")
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

# Bullet aus der Quelle, alles Überflüssige aus. Picking braucht nur die Bullet-2-
# Kollisionswelt (BulletCollision + LinearMath); EXCLUDE_FROM_ALL hält den Rest aus
# dem Standard-Build.
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
# Bullet 3.25 deklariert noch cmake_minimum_required < 3.5, was CMake 4.x ablehnt.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
add_subdirectory(external/bullet3 EXCLUDE_FROM_ALL)
target_link_libraries(decade PRIVATE BulletCollision LinearMath)
set_target_properties(BulletCollision LinearMath PROPERTIES SYSTEM TRUE)

add_subdirectory(external/embed-resource)
target_include_directories(decade SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/embed-resource)
# embed-resource läuft während des Builds. Instrumentiert würde ein Befund darin
# den Build abbrechen, statt das Programm zu prüfen — also vom Sanitizer ausnehmen.
target_compile_options(embed-resource PRIVATE -fno-sanitize=all)
target_link_options(embed-resource PRIVATE -fno-sanitize=all)

# --- Eingebettete Ressourcen (Name hinter der Datei = Zugriffsname im Code) ----

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

option(DECADE_BUILD_TESTS "Unit-Tests bauen (tests/ spiegelt src/)" ON)
if(DECADE_BUILD_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif()

# --- Gates --------------------------------------------------------------------
# Absichtlich nicht Teil des Standard-Builds, damit normale Compiles schnell
# bleiben; explizit oder in CI aufrufen. Befehle in betrieb.md.

# clang-tidy auf der einzigen Translation Unit — sie deckt via HeaderFilterRegex
# alle src/-Header ab. --extra-arg: -include type_traits umgeht einen Include-Bug
# in wx/meta/convertible.h, die -Wno-* entschärfen GCC-Flags aus compile_commands.json.
add_custom_target(clang-tidy
	COMMAND clang-tidy -p ${CMAKE_BINARY_DIR}
		--warnings-as-errors=*
		--extra-arg=-include --extra-arg=type_traits
		--extra-arg=-Wno-error --extra-arg=-Wno-unknown-warning-option
		src/application/main.cpp
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	COMMENT "clang-tidy-Gate (jeder Befund = Fehler) auf src/application/main.cpp"
	VERBATIM USES_TERMINAL)

# Sanitizer verlangen andere Compile-Flags als der Normalbau, also konfiguriert
# jedes Gate einen eigenen Build-Ordner, baut alles darin neu und lässt die
# Testsuite laufen — nur ein Lauf findet Fehler, ein Bau allein nicht. Schlägt ein
# Sanitizer an, endet der Prozess ungleich null und ctest bricht. Die Flags stehen
# in CMAKE_CXX_FLAGS, damit sie jedes Target im Ordner treffen, auch Bullet.
# UBSan meldet sonst nur und läuft weiter — ohne -fno-sanitize-recover bliebe das
# Gate grün; -fno-omit-frame-pointer rettet die Stacktraces.
# https://gcc.gnu.org/onlinedocs/gcc-9.2.0/gcc/Instrumentation-Options.html
# LeakSanitizer steckt unter Linux schon in AddressSanitizer; hier trotzdem genannt.
add_custom_target(sanitize-address
	COMMAND ${CMAKE_COMMAND} -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_BINARY_DIR}/san-address -G ${CMAKE_GENERATOR}
		-DCMAKE_BUILD_TYPE=RelWithDebInfo
		"-DCMAKE_CXX_FLAGS=-fsanitize=address,leak,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer"
	COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/san-address
	COMMAND ${CMAKE_COMMAND} -E env UBSAN_OPTIONS=print_stacktrace=1
		${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR}/san-address --output-on-failure
	COMMENT "Sanitizer-Gate -fsanitize=address,leak,undefined (Bau + Testlauf)"
	VERBATIM USES_TERMINAL)

# MemorySanitizer schliesst AddressSanitizer aus (der Compiler weist die Kombination
# ab) und kennt nur clang — darum ein zweites Gate. Er verlangt, dass ALLE
# Abhängigkeiten instrumentiert sind; mit dem System-libstdc++ und -gtest meldet er
# Fehlalarme aus fremdem Code. Diagnosewerkzeug, kein CI-Gate — siehe betrieb.md.
add_custom_target(sanitize-memory
	COMMAND ${CMAKE_COMMAND} -S ${CMAKE_SOURCE_DIR} -B ${CMAKE_BINARY_DIR}/san-memory -G ${CMAKE_GENERATOR}
		-DCMAKE_BUILD_TYPE=RelWithDebInfo
		-DCMAKE_CXX_COMPILER=clang++
		"-DCMAKE_CXX_FLAGS=-fsanitize=memory -fno-omit-frame-pointer"
	COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/san-memory
	COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR}/san-memory --output-on-failure
	COMMENT "Sanitizer-Gate -fsanitize=memory (Bau + Testlauf, clang)"
	VERBATIM USES_TERMINAL)
