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

# Link with other targets.
target_link_libraries(${PROJECT_NAME} PRIVATE
    api
    event
    engine
    exchange
    cta_strategy
    r_break_strategy
)
if (CMAKE_GENERATOR MATCHES "Visual Studio")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    )
endif()

if(UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)
endif()