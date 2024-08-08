set(PROJECT_NAME cta_strategy)

set(Header_Files
    "base.h"
    "cta_strategy.h"
    "engine.h"
    "template.h"
)
source_group("Header Files" FILES ${Header_Files})

set(Source_Files
    "base.cpp"
    "cta_strategy.cpp"
    "engine.cpp"
    "template.cpp"
)
source_group("Source Files" FILES ${Source_Files})

set(ALL_FILES
    ${Header_Files}
    ${Source_Files}
)

add_library(${PROJECT_NAME} SHARED ${ALL_FILES})
init_target(${PROJECT_NAME} "app")

target_compile_definitions(${PROJECT_NAME} PRIVATE KEEN_BUILD_APP_LIB)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${src_loc}/core
    ${libs_loc}/date/include
    ${libs_loc}/dylib/include
    ${libs_loc}/nlohmann_json/include
)

set(ADDITIONAL_LIBRARY_DEPENDENCIES
    api
    engine
)
target_link_libraries(${PROJECT_NAME} PUBLIC ${ADDITIONAL_LIBRARY_DEPENDENCIES})
