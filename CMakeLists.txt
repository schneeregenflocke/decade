cmake_minimum_required (VERSION 3.8)

project(Decade)

message("CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
message("cmake version: ${CMAKE_VERSION}")

function(message_list list_name)
	message(${list_name})
	foreach(item IN LISTS ${list_name})
		message(STATUS ${item})
	endforeach()
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


set(wxUSE_GUI ON CACHE BOOL "wxUSE_GUI" FORCE)
#set(wxBUILD_CUSTOM_SETUP_HEADER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/wx_custom_setup" CACHE PATH "wxBUILD_CUSTOM_SETUP_HEADER_PATH" FORCE)
set(wxUSE_WINRT OFF CACHE BOOL "wxUSE_WINRT" FORCE)
#set(wxUSE_CAIRO OFF CACHE BOOL "wxUSE_CAIRO" FORCE)
#set(wxUSE_NO_RTTI ON CACHE BOOL "wxUSE_NO_RTTI" FORCE)
set(wxUSE_EXCEPTIONS OFF CACHE BOOL "wxUSE_EXCEPTIONS" FORCE)
#set(wxUSE_ON_FATAL_EXCEPTION ON CACHE BOOL "wxUSE_ON_FATAL_EXCEPTION" FORCE)
set(wxUSE_STL ON CACHE BOOL "wxUSE_STL" FORCE)
set(wxUSE_CAIRO OFF CACHE BOOL "wxUSE_CAIRO" FORCE)

set(wxUSE_OPENGL ON CACHE BOOL "wxUSE_OPENGL" FORCE)

if(${CMAKE_SYSTEM_NAME} STREQUAL Windows)
	set(wxBUILD_TOOLKIT msw CACHE STRING "wxBUILD_TOOLKIT" FORCE)
else()
	# set(wxBUILD_TOOLKIT gtk3 CACHE STRING "wxBUILD_TOOLKIT" FORCE)
	# cc1plus: Warnung: /home/titan99/code/decade-build/external/wxWidgets/libs/core/cotire/core_CXX_prefix.hxx.gch: nicht verwendet, da »__WXGTK2__« nicht definiert [-Winvalid-pch]
endif()

set(wxBUILD_SHARED OFF CACHE BOOL "wxBUILD_SHARED" FORCE)
set(wxBUILD_PRECOMP ON CACHE BOOL "wxBUILD_PRECOMP" FORCE)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/wxWidgets")
#target_compile_definitions(gl   PUBLIC _CRT_SECURE_NO_WARNINGS)
#target_compile_definitions(core PUBLIC _CRT_SECURE_NO_WARNINGS)
#target_compile_definitions(base PUBLIC _CRT_SECURE_NO_WARNINGS)
include_directories("${CMAKE_CURRENT_BINARY_DIR}/external/wxWidgets/mswud")


set(GLAD_PROFILE "core" CACHE STRING "OpenGL profile" FORCE)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/glad")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/glm")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/freetype2/include")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/freetype2")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/lodepng")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/csv/include")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/csv")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/pugixml/include")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/pugixml")
target_compile_definitions(pugixml PUBLIC PUGIXML_WCHAR_MODE)


include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/embed-resource")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/embed-resource")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/external/sigslot/include")


file(GLOB_RECURSE resource_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS shaders/*.glsl)

#file(RELATIVE_PATH relative_boost_license_path ${CMAKE_SOURCE_DIR} ${Boost_INCLUDE_DIRS}/LICENSE_1_0.txt)
#${relative_boost_license_path}

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

list(APPEND source_files ${resources})
list(APPEND source_files external/lodepng/lodepng.h external/lodepng/lodepng.cpp)


if(WIN32)
  list(APPEND source_files "${CMAKE_CURRENT_SOURCE_DIR}/external/wxWidgets/include/wx/msw/amd64_dpi_aware.manifest")
endif(WIN32)

message_list(source_files)


add_executable(Decade ${source_files})

target_link_libraries(Decade glad)
target_link_libraries(Decade freetype)
target_link_libraries(Decade gl core base)
target_link_libraries(Decade pugixml)


