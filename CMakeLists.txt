cmake_minimum_required (VERSION 3.8)
set(CMAKE_CXX_STANDARD 17)

add_compile_definitions(HAVE_TYPE_TRAITS)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(Decade)
add_executable(Decade)



message("CMAKE_VERSION: ${CMAKE_VERSION}")
message("CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
message("CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")


function(message_list list_name)
	message("List ${list_name}")
	foreach(item IN LISTS ${list_name})
		message(STATUS ${item})
	endforeach()
	message("End List ${list_name}")
endfunction()



if(MSVC)
	add_compile_options(/Zc:__cplusplus)
endif()



set(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS date_time)

message("Boost_FOUND: ${Boost_FOUND}")
if(NOT ${Boost_FOUND})
	message(FATAL_ERROR "Set directory path variable Boost_DIR to the directory containing the file BoostConfig.cmake (boost/stage/lib/cmake/Boost-1.74.0)")
endif()

message("Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
message("Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")
message("Boost_LIBRARIES: ${Boost_LIBRARIES}")

target_include_directories(Decade PRIVATE ${Boost_INCLUDE_DIRS})
#target_link_directories(Decade PRIVATE ${Boost_LIBRARY_DIRS})
target_link_libraries(Decade PRIVATE ${Boost_LIBRARIES})



set(wxBUILD_SHARED OFF CACHE BOOL "wxBUILD_SHARED" FORCE)
set(wxUSE_STL ON CACHE BOOL "wxUSE_STL" FORCE)
set(wxUSE_GUI ON CACHE BOOL "wxUSE_GUI" FORCE)
set(wxUSE_OPENGL ON CACHE BOOL "wxUSE_OPENGL" FORCE)
set(wxUSE_WINRT	OFF CACHE BOOL "wxUSE_WINRT" FORCE)
set(wxUSE_EXCEPTIONS OFF CACHE BOOL "wxUSE_EXCEPTIONS" FORCE)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/wxWidgets" EXCLUDE_FROM_ALL)
#fixed library names https://github.com/wxWidgets/wxWidgets/commit/0f806ad96db4e22053769c0b2f3b7be4026ea8de
target_link_libraries(Decade PRIVATE wxgl wxcore wxbase)


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


file(GLOB_RECURSE resource_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS shaders/*.glsl)
list(APPEND resource_files
	LICENSE.txt
	external/csv/LICENSE
	external/freetype2/docs/LICENSE.TXT
#	external/embed_resource/LICENSE.txt		# ninja : error : dependency cycle: 
	external/glad/LICENSE
	external/glm/copying.txt
	external/lodepng/LICENSE
	external/pugixml/LICENSE.md
#	external/wxWidgets/licence.txt			# ninja : error : dependency cycle: external/wxWidgets/licence.txt.cpp -> external/wxWidgets/licence.txt.cpp
	external/provisional_license_collection/Boost_LICENSE_1_0.txt
	external/provisional_license_collection/embed-resource-LICENSE.txt
	external/provisional_license_collection/wxWidgets-licence.txt
	external/sigslot/LICENSE
)
#message_list(resource_files)
embed_resources(resources ${resource_files})
target_sources(Decade PRIVATE ${resources})


file(GLOB_RECURSE source_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS src/*.h src/*.cpp)
#message_list(source_files)
target_sources(Decade PRIVATE ${source_files})


if(${CMAKE_SYSTEM_NAME} STREQUAL Windows)
	target_sources(Decade PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/external/wxWidgets/include/wx/msw/amd64_dpi_aware.manifest")
endif()

