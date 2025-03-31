# src/core/CMakeLists.txt
find_path(MVS_INCLUDE_DIR               # 定义变量名，用于存储找到的头文件路径
    NAMES MvCameraControl.h             # 要查找的头文件名
    PATHS "${MVS_SDK_ROOT}/Includes"    # 搜索路径
    NO_DEFAULT_PATH                     # 禁止在默认系统路径中搜索
)

find_library(MVS_LIBRARY
    NAMES MvCameraControl
    PATHS "${MVS_SDK_ROOT}/Libraries/${ARCH_TYPE}"
    NO_DEFAULT_PATH
)

message(STATUS "MVS_INCLUDE_DIR: ${MVS_INCLUDE_DIR}")
message(STATUS "MVS_LIBRARY: ${MVS_LIBRARY}")

# 相对路径
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h")

add_library(CoreModule STATIC
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(CoreModule PUBLIC
    ${MVS_INCLUDE_DIR}
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(CoreModule PRIVATE
    ${MVS_LIBRARY}
    Qt${QT_VERSION_MAJOR}::Widgets
    ModelModule
)