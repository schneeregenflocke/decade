cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)

project(Decade)

message("CMAKE_VERSION: ${CMAKE_VERSION}")
message("CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
message("CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")


add_compile_definitions(HAVE_TYPE_TRAITS)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(Decade)


if(MSVC)
	add_compile_options(/Zc:__cplusplus)
endif()
target_compile_definitions(Decade PUBLIC _CRT_SECURE_NO_WARNINGS)


set(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS date_time)

message("Boost_FOUND: ${Boost_FOUND}")
if(NOT ${Boost_FOUND})
	message(FATAL_ERROR "Set directory path variable Boost_DIR to the directory containing the file BoostConfig.cmake (e.g. boost/stage/lib/cmake/Boost-1.74.0)")
endif()

message("Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
message("Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")
message("Boost_LIBRARIES: ${Boost_LIBRARIES}")

target_include_directories(Decade PRIVATE ${Boost_INCLUDE_DIRS})
#target_link_directories(Decade PRIVATE ${Boost_LIBRARY_DIRS})
target_link_libraries(Decade PRIVATE ${Boost_LIBRARIES})


#WxWidgets ON
set(wxUSE_STL ON CACHE BOOL "wxUSE_STL" FORCE)
set(wxUSE_GUI ON CACHE BOOL "wxUSE_GUI" FORCE)
set(wxUSE_OPENGL ON CACHE BOOL "wxUSE_OPENGL" FORCE)

#WxWidgets OFF
set(wxBUILD_SHARED OFF CACHE BOOL "wxBUILD_SHARED" FORCE)
set(wxUSE_WINRT	OFF CACHE BOOL "wxUSE_WINRT" FORCE)
set(wxUSE_EXCEPTIONS OFF CACHE BOOL "wxUSE_EXCEPTIONS" FORCE)



add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/wxWidgets" EXCLUDE_FROM_ALL)
#fixed library names https://github.com/wxWidgets/wxWidgets/commit/0f806ad96db4e22053769c0b2f3b7be4026ea8de
target_link_libraries(Decade PRIVATE wxpropgrid wxgl wxcore wxbase)


set(GLAD_PROFILE "core" CACHE STRING "OpenGL profile" FORCE)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/glad" EXCLUDE_FROM_ALL)
target_link_libraries(Decade PRIVATE glad)


target_include_directories(Decade PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/external/glm")


target_include_directories(Decade PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/external/freetype2/include")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/freetype2" EXCLUDE_FROM_ALL)
target_link_libraries(Decade PRIVATE freetype)


target_include_directories(Decade PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/external/lodepng")
target_sources(Decade PRIVATE external/lodepng/lodepng.h external/lodepng/lodepng.cpp)


target_include_directories(Decade PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/external/csv/include")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/csv" EXCLUDE_FROM_ALL)


add_compile_definitions(PUGIXML_WCHAR_MODE)
target_include_directories(Decade PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/external/pugixml/include")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/pugixml" EXCLUDE_FROM_ALL)
target_link_libraries(Decade PRIVATE pugixml)


target_include_directories(Decade PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/external/embed-resource")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/embed-resource" EXCLUDE_FROM_ALL)


target_include_directories(Decade PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/external/sigslot/include")


#file(GLOB RESOURCE_FILES_PATHS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS shaders/*.glsl)
file(GLOB RESOURCE_FILES_PATHS CONFIGURE_DEPENDS shaders/*.glsl)

list(APPEND LICENSE_FILES_PATHS
	"${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt"
	"${CMAKE_CURRENT_SOURCE_DIR}/external/csv/LICENSE"
	"${CMAKE_CURRENT_SOURCE_DIR}/external/freetype2/docs/LICENSE.TXT"
	"${CMAKE_CURRENT_SOURCE_DIR}/external/embed-resource/LICENSE.txt"		
	"${CMAKE_CURRENT_SOURCE_DIR}/external/glad/LICENSE"
	"${CMAKE_CURRENT_SOURCE_DIR}/external/glm/copying.txt"
	"${CMAKE_CURRENT_SOURCE_DIR}/external/lodepng/LICENSE"
	"${CMAKE_CURRENT_SOURCE_DIR}/external/pugixml/LICENSE.md"
	"${CMAKE_CURRENT_SOURCE_DIR}/external/wxWidgets/docs/licence.txt"
	"${CMAKE_CURRENT_SOURCE_DIR}/external/sigslot/LICENSE"
	"${Boost_INCLUDE_DIRS}/LICENSE_1_0.txt"
)
message("Resource Files: ${RESOURCE_FILES_PATHS} ${LICENSE_FILES_PATHS}")
embed_resources(RESOURCES ${RESOURCE_FILES_PATHS} ${LICENSE_FILES_PATHS})
target_sources(Decade PRIVATE ${RESOURCES})


file(GLOB_RECURSE source_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS src/*.h src/*.cpp)
#message_list(source_files)
target_sources(Decade PRIVATE ${source_files})


if(${CMAKE_SYSTEM_NAME} STREQUAL Windows)
	target_sources(Decade PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/external/wxWidgets/include/wx/msw/amd64_dpi_aware.manifest")
endif()

