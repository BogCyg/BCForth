cmake_minimum_required( VERSION 3.10 )

# For a new project it is sufficient to change only its name in the following line
set( PROJECT_NAME BCForth )

project( ${PROJECT_NAME} )

set( CMAKE_BUILD_TYPE Debug )
#set( CMAKE_BUILD_TYPE Release )



if( WIN32 )
	set( CMAKE_CXX_FLAGS "/DWIN32 /D_WINDOWS /W3 /Ox /GR /EHsc /std:c++latest /D_UNICODE /DUNICODE" )
	set( CMAKE_CXX_FLAGS_DEBUG "/MDd /Zi /Ob0 /Od /RTC1 /std:c++latest /D_UNICODE /DUNICODE" )
	message( "Win settings chosen..." )
elseif( ${CMAKE_SYSTEM_NAME} STREQUAL "Darwin" )
	set( CMAKE_CXX_FLAGS "-std=c++2a -Wall" )
	set( CMAKE_CXX_FLAGS_DEBUG "-g -std=c++2a -Wall" )
	message( "Mac settings chosen..." )
elseif( UNIX )
	set( CMAKE_CXX_FLAGS "-std=c++2a -O3 -fconcepts -Wfatal-errors" )
	set( CMAKE_CXX_FLAGS_DEBUG "-g -std=c++2a -fconcepts -Wfatal-errors" )
	message( "Linux settings chosen..." )
endif()


# Inform CMake where the header files are
include_directories( include include/Auxiliary include/Interfaces include/Modules include/Words )


# Automatically add all *.cpp and *.h files to the project
file ( GLOB_RECURSE SOURCES "./src/*" "./include/*" )
add_executable( ${PROJECT_NAME} ${SOURCES} )


# https://stackoverflow.com/questions/31422680/how-to-set-visual-studio-filters-for-nested-sub-directory-using-cmake
# Build the folder(s) tree following source structure (tree root at CMAKE_CURRENT_SOURCE_DIR).
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
set_property( GLOBAL PROPERTY USE_FOLDERS ON )



# Set the default project 
set_property( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME} )


message( "CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}" )
