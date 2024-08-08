set(PROJECT_NAME engine)

source_group("database" FILES ${database})

set(Header_Files
    "app.h"
    "constant.h"
    "converter.h"
    "engine.h"
    "event.h"
    "exchange.h"
    "object.h"
    "settings.h"
    "utility.h"
)
source_group("Header Files" FILES ${Header_Files})

set(Source_Files
    "app.cpp"
    "constant.cpp"
    "converter.cpp"
    "engine.cpp"
    "event.cpp"
    "exchange.cpp"
    "object.cpp"
    "settings.cpp"
    "utility.cpp"
)
source_group("Source Files" FILES ${Source_Files})

set(ALL_FILES
    ${Header_Files}
    ${Source_Files}
)

add_library(${PROJECT_NAME} SHARED ${ALL_FILES})
init_target(${PROJECT_NAME} "core")

target_compile_definitions(${PROJECT_NAME} PRIVATE KEEN_BUILD_TRADER_LIB)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${src_loc}/core
    ${libs_loc}/date/include
    ${libs_loc}/magic_enum/include
    ${libs_loc}/nlohmann_json/include
)

add_dependencies(${PROJECT_NAME}
    api
)

set(ADDITIONAL_LIBRARY_DEPENDENCIES
    api
)

target_link_libraries(${PROJECT_NAME} PUBLIC ${ADDITIONAL_LIBRARY_DEPENDENCIES})