set(PROJECT_NAME api)

set(Source_Files
    "DateTime.cpp"
    "DateTime.h"
    "Defines.h"
    "EventLoop.cpp"
    "EventLoop.h"
    "Globals.cpp"
    "Globals.h"
    "Logger.cpp"
    "Logger.h"
    "LoggerListeners.cpp"
    "LoggerListeners.h"
    "LoggerSimple.h"
    "RestClient.cpp"
    "RestClient.h"
    "StringUtils.cpp"
    "StringUtils.h"
    "WebsocketClient.cpp"
    "WebsocketClient.h"
)
source_group("Source Files" FILES ${Source_Files})

set(OSSupport
	"OSSupport/ConsoleSignalHandler.cpp"
	"OSSupport/ConsoleSignalHandler.h"
    "OSSupport/CriticalSection.cpp"
    "OSSupport/CriticalSection.h"
    "OSSupport/Event.cpp"
    "OSSupport/Event.h"
    "OSSupport/File.cpp"
    "OSSupport/File.h"
    "OSSupport/Singleton.h"
    "OSSupport/StackTrace.cpp"
    "OSSupport/StackTrace.h"
    "OSSupport/ThreadPool.h"
    "OSSupport/WinStackWalker.cpp"
    "OSSupport/WinStackWalker.h"
)

source_group("OSSupport" FILES ${OSSupport})

set(HttpClient
    "HttpClient/Sender.cpp"
    "HttpClient/Sender.h"
    "HttpClient/Callback.h"
    "HttpClient/Model.h"
)

source_group("HttpClient" FILES ${HttpClient})

set(ALL_FILES
    ${Source_Files}
    ${OSSupport}
    ${HttpClient}
)

add_library(${PROJECT_NAME} SHARED ${ALL_FILES})
init_target(${PROJECT_NAME} "core")

target_compile_definitions(${PROJECT_NAME} PRIVATE KEEN_BUILD_API_LIB)

set(ROOT_NAMESPACE api)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${src_loc}/core/api
    ${libs_loc}/cpp-httplib
    ${libs_loc}/date/include
    ${libs_loc}/websocketpp
    ${libs_loc}/asio/asio/include
    ${libs_loc}/nlohmann_json/include
)

if(OPENSSL_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${OPENSSL_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENSSL_LIBRARIES})
else()
    message(FATAL_ERROR "OpenSSL not found")
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt)