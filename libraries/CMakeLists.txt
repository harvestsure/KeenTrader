

# 配置 libhv 选项
set(BUILD_SHARED OFF)       # 不构建共享库
set(BUILD_STATIC ON)        # 构建静态库
set(BUILD_EXAMPLES OFF)     # 不构建测试
set(BUILD_UNITTEST OFF)     # 不构建示例

# add_subdirectory(libhv)


# set(JSON_BuildTests OFF CACHE INTERNAL "")
# add_subdirectory(nlohmann_json)

add_subdirectory(fmt)
init_target(fmt "libraries")