cmake_minimum_required(VERSION 3.16.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(KeenTrader C CXX)

include(cmake/init_target.cmake)
include(cmake/options.cmake)
include(cmake/variables.cmake)
include(cmake/dependents.cmake)

# Enable Clang-Tidy static analysis
find_program(CLANG_TIDY clang-tidy)
if(CLANG_TIDY AND NOT MSVC)
    set(CMAKE_CXX_CLANG_TIDY 
        ${CLANG_TIDY}
        -header-filter=.*
        -checks=-*,clang-diagnostic-*,clang-analyzer-*,bugprone-*,modernize-*,performance-*
        --fix
    )
    message(STATUS "Clang-Tidy enabled")
else()
    if(CLANG_TIDY)
        message(STATUS "Clang-Tidy found but disabled on MSVC")
    else()
        message(STATUS "Clang-Tidy not found")
    endif()
endif()

add_subdirectory(core)
add_subdirectory(app)
add_subdirectory(examples)
add_subdirectory(libraries)


if (MSVC)
	 # Set the startup project .
     set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT trader)
endif (MSVC)