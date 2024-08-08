set(PROJECT_NAME exchange)

set(Header_Files
    "algo_hmac.h"
    "binance_exchange.h"
    "okx_exchange.h"
    "utils.h"
)
source_group("Header Files" FILES ${Header_Files})

set(Source_Files
    "algo_hmac.cpp"
    "binance_exchange.cpp"
    "okx_exchange.cpp"
    "utils.cpp"
)
source_group("Source Files" FILES ${Source_Files})

set(ALL_FILES
    ${Header_Files}
    ${Source_Files}
)

add_library(${PROJECT_NAME} SHARED ${ALL_FILES})
init_target(${PROJECT_NAME} "core")

target_compile_definitions(${PROJECT_NAME} PRIVATE KEEN_BUILD_EXCHANGE_LIB)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${src_loc}/core
    ${libs_loc}/nlohmann_json/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    api
    event
    engine
)


if(OPENSSL_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${OPENSSL_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENSSL_LIBRARIES})
else()
    message(FATAL_ERROR "OpenSSL not found")
endif()