# \CMakeLists.txt
# 基础配置 ---------------------------------------------------
cmake_minimum_required(VERSION 3.21)# 最低版本要求
project(MVS_Subdevelop 
    VERSION 0.1 
    LANGUAGES CXX
    DESCRIPTION "Industrial Camera Control Suite"
)
# 全局配置 ---------------------------------------------------
set(CMAKE_CXX_STANDARD 17)          # 支持C++17
set(CMAKE_CXX_STANDARD_REQUIRED ON) # 强制使用C++17
set(CMAKE_AUTOMOC ON)               # 自动生成MOC文件
set(CMAKE_AUTORCC ON)               # 自动生成RCC文件
set(CMAKE_AUTOUIC ON)               # 自动生成UI文件

# 架构检测 ---------------------------------------------------
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_TYPE "win64")
else()
    set(ARCH_TYPE "win32")
endif()
message(STATUS "Target Architecture: ${ARCH_TYPE}")

# Qt配置 -----------------------------------------------------
find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)
# SDK配置 ----------------------------------------------------
if(NOT DEFINED ENV{MVCAM_COMMON_RUNENV})
    message(FATAL_ERROR "MVCAM_COMMON_RUNENV environment variable not set!")
endif()
set(MVS_SDK_ROOT "$ENV{MVCAM_COMMON_RUNENV}")

# 模块化子目录 -----------------------------------------------
add_subdirectory(src/core)          # 核心模块
add_subdirectory(src/gui)           # GUI模块
add_subdirectory(src/model)         # 数据模型模块

# 可执行目标 -------------------------------------------------
add_executable(${PROJECT_NAME}
    src/app/main.cpp                # 主函数入口
)

# 链接依赖项 -------------------------------------------------
target_link_libraries(${PROJECT_NAME} PRIVATE
    CoreModule
    GuiModule
    ModelModule
    Qt${QT_VERSION_MAJOR}::Widgets
)

# 安装配置 ---------------------------------------------------
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION bin
    LIBRARY DESTINATION lib
)
