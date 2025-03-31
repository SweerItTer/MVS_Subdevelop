# src\model\CMakeLists.txt

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h")
# 数据模型模块
add_library(ModelModule STATIC
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(ModelModule PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(ModelModule PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    CoreModule
)