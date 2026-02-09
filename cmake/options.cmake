
option(BUILD_SHARED "build shared library" ON)
option(BUILD_STATIC "build static library" OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>)


if(MSVC)
    if(BUILD_STATIC)
        add_compile_options(
            $<$<CONFIG:Debug>:/MTd>
            $<$<CONFIG:Release>:/MT>
            $<$<CONFIG:MinSizeRel>:/MT>
            $<$<CONFIG:RelWithDebInfo>:/MT>
        )
        else()
        add_compile_options(
            $<$<CONFIG:Debug>:/MDd>
            $<$<CONFIG:Release>:/MD>
            $<$<CONFIG:MinSizeRel>:/MD>
            $<$<CONFIG:RelWithDebInfo>:/MD>
        )
    endif()

    # Make build use multiple threads under MSVC:
    add_compile_options(/MP)

    # Make build use Unicode:
    add_compile_definitions(UNICODE _UNICODE)

    # Turn off CRT warnings:
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    # add_compile_options(/W4 /WX)
else()
    # Enable strict warnings but allow issues common in third-party libraries
    add_compile_options(
        -Wall -Wextra -Wpedantic -Werror
        -Wshadow                           # Warn about variable shadowing
        -Wconversion                       # Warn about implicit type conversions
        -Wno-error=unused-parameter
        -Wno-error=unused-variable
        -Wno-error=conversion              # Type conversion warnings from third-party libs
        -Wno-error=shadow                  # Variable shadowing in third-party libs
    )
    
    # Clang-specific options
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-Wno-error=null-pointer-subtraction)
    endif()
endif()
