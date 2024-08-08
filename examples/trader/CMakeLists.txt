set(PROJECT_NAME trader)

set(Source_Files
    "main.cpp"
)
source_group("Source Files" FILES ${Source_Files})

set(ALL_FILES
    ${Source_Files}
)

add_executable(${PROJECT_NAME} ${ALL_FILES})
init_target(${PROJECT_NAME} "examples")

target_include_directories(${PROJECT_NAME} PRIVATE
    ${src_loc}
    ${src_loc}/core
    ${libs_loc}/nlohmann_json/include
)

add_dependencies(${PROJECT_NAME}
    api
    engine
    r_break_strategy
)

# Link with other targets.
target_link_libraries(${PROJECT_NAME} PRIVATE
    cta_strategy
    exchange
)

set(ADDITIONAL_LIBRARY_DEPENDENCIES
    engine
)
target_link_libraries(${PROJECT_NAME} PRIVATE ${ADDITIONAL_LIBRARY_DEPENDENCIES})
