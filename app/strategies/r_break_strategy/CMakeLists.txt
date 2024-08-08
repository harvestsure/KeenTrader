set(PROJECT_NAME r_break_strategy)

set(Header_Files
    "ArrayManager.h"
    "r_break_strategy.h"
)
source_group("Header Files" FILES ${Header_Files})

set(Source_Files
    "ArrayManager.cpp"
    "r_break_strategy.cpp"
)
source_group("Source Files" FILES ${Source_Files})

set(ALL_FILES
    ${Header_Files}
    ${Source_Files}
)

add_library(${PROJECT_NAME} SHARED ${ALL_FILES})
init_target(${PROJECT_NAME} "app/strategies")

target_include_directories(${PROJECT_NAME} PRIVATE
    ${src_loc}
    ${src_loc}/core
    ${libs_loc}/date/include
    ${libs_loc}/nlohmann_json/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    api    
    event
    engine
    cta_strategy
)