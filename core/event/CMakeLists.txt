set(PROJECT_NAME event)

set(Source_Files
    "event.h"
    "event.cpp"
)
source_group("Source Files" FILES ${Source_Files})

set(ALL_FILES
    ${Source_Files}
)

add_library(${PROJECT_NAME} SHARED ${ALL_FILES})
init_target(${PROJECT_NAME} "core")

target_compile_definitions(${PROJECT_NAME} PRIVATE KEEN_BUILD_EVENT_LIB)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${src_loc}/core
    ${libs_loc}/nlohmann_json/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    api
)