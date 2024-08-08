
option(BUILD_SHARED "build shared library" ON)
option(BUILD_STATIC "build static library" OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin/RelWithDebInfo)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/bin/MinSizeRel)

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
endif()