cmake_minimum_required (VERSION 3.8)

project(Decade)

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


set(CMAKE_CXX_STANDARD 17)

if(MSVC)
	add_compile_options(/Zc:__cplusplus)
endif()


set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

message("Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
message("Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")


set(wxBUILD_SHARED OFF CACHE BOOL "wxBUILD_SHARED" FORCE)
set(wxUSE_STL ON CACHE BOOL "wxUSE_STL" FORCE)
set(wxUSE_GUI ON CACHE BOOL "wxUSE_GUI" FORCE)
set(wxUSE_OPENGL ON	CACHE BOOL "wxUSE_OPENGL" FORCE)
set(wxUSE_WINRT	OFF CACHE BOOL "wxUSE_WINRT" FORCE)
set(wxUSE_EXCEPTIONS OFF CACHE BOOL "wxUSE_EXCEPTIONS" FORCE)
set(wxUSE_CAIRO	OFF CACHE BOOL "wxUSE_CAIRO" FORCE)
#set(wxBUILD_CUSTOM_SETUP_HEADER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/wxWidgets/include/msvc/" CACHE PATH "wxBUILD_CUSTOM_SETUP_HEADER_PATH" FORCE)
#set(wxUSE_NO_RTTI ON CACHE BOOL "wxUSE_NO_RTTI" FORCE)
#set(wxUSE_ON_FATAL_EXCEPTION ON CACHE BOOL "wxUSE_ON_FATAL_EXCEPTION" FORCE)

if(${CMAKE_SYSTEM_NAME} STREQUAL Windows)
	set(wxBUILD_TOOLKIT msw CACHE STRING "wxBUILD_TOOLKIT" FORCE)
else()
	# set(wxBUILD_TOOLKIT gtk3 CACHE STRING "wxBUILD_TOOLKIT" FORCE)
	# cc1plus: Warnung: /home/titan99/code/decade-build/external/wxWidgets/libs/core/cotire/core_CXX_prefix.hxx.gch: nicht verwendet, da »__WXGTK2__« nicht definiert [-Winvalid-pch]
endif()

#if(${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
#	set(wxBUILD_PRECOMP OFF CACHE BOOL "wxBUILD_PRECOMP" FORCE)
#else()
	set(wxBUILD_PRECOMP ON CACHE BOOL "wxBUILD_PRECOMP" FORCE)
#endif()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/wxWidgets/include")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/wxWidgets")



set(GLAD_PROFILE "core" CACHE STRING "OpenGL profile" FORCE)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/glad")


include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/glm")


include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/freetype2/include")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/freetype2")


include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/lodepng")


include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/csv/include")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/csv")

#-w dupbuild=err
add_compile_definitions(PUGIXML_WCHAR_MODE)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/pugixml/include")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/pugixml")



include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/embed-resource")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/embed-resource")


include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/sigslot/include")


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

message_list(resource_files)
embed_resources(resources ${resource_files})


file(GLOB_RECURSE source_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS src/*.h src/*.cpp)
message_list(source_files)

add_executable(Decade ${source_files})
target_compile_definitions(Decade PUBLIC _CRT_SECURE_NO_WARNINGS)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
	target_sources(Decade PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/external/wxWidgets/include/wx/msw/amd64_dpi_aware.manifest")
endif()

target_sources(Decade PRIVATE ${resources})
target_sources(Decade PRIVATE external/lodepng/lodepng.h external/lodepng/lodepng.cpp)


target_link_libraries(Decade glad)
target_link_libraries(Decade freetype)
target_link_libraries(Decade gl core base)
target_link_libraries(Decade pugixml)
