cmake_minimum_required (VERSION 3.10)
set(CMAKE_CXX_STANDARD 20)

if (MSVC)
add_compile_options("/utf-8")
endif()

project ("ProjectSnow")

# 查找OpenCV包
find_package(OpenCV REQUIRED)

# 包含OpenCV头文件目录
include_directories(${OpenCV_INCLUDE_DIRS})
add_executable (ProjectSnow "main.cpp"  "src/ScreenCapture.cpp")

# target_compile_definitions(ProjectSnow PRIVATE UNICODE _UNICODE)

# 链接OpenCV库
target_link_libraries(ProjectSnow ${OpenCV_LIBS})