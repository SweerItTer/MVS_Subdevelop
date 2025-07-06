# src\gui\CMakeLists.txt

# 资源文件配置
set(QRC_FILES
    # resources/icons.qrc
)
# 坑: 默认 CMAKE_AUTOUIC_SEARCH_PATHS 为 CMAKE_CURRENT_SOURCE_DIR,需要手动替换为 ui 目录
set(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/ui")    # 设置自动生成的头文件搜索路径

file(GLOB_RECURSE UI_FILES "${CMAKE_CURRENT_SOURCE_DIR}/ui/*.ui")   # 自动查找 ui 文件
file(GLOB_RECURSE SOURCES "src/*.cpp")     # 源文件路径
file(GLOB_RECURSE HEADERS "include/*.h")   # 头文件路径

# GUI模块库
add_library(GuiModule STATIC
    ${SOURCES}
    ${HEADERS}
    ${UI_FILES}
    ${QRC_FILES}
)

target_include_directories(GuiModule PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>  # 用于构建时的头文件搜索路径
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>          # 添加构建目录，用于存放生成的头文件
    $<INSTALL_INTERFACE:include>                            # 用于安装时的头文件搜索路径
)

target_link_libraries(GuiModule PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Network
    CoreModule
    ModelModule
)
