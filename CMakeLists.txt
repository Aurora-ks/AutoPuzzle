cmake_minimum_required (VERSION 3.10)
set(CMAKE_CXX_STANDARD 20)

if (MSVC)
    add_compile_options("/utf-8" "/permissive-")
endif()

project ("PSA")
include_directories("src")
#add_definitions(-DUNICODE -D_UNICODE)

# 查找OpenCV包
find_package(OpenCV REQUIRED)

# 包含OpenCV头文件目录
include_directories(${OpenCV_INCLUDE_DIRS})
add_executable (PSA "main.cpp"  "src/ScreenCapture.cpp" "src/recognition.cpp" "src/utils.cpp" "src/ScreenController.cpp")

# 链接OpenCV库
target_link_libraries(PSA ${OpenCV_LIBS})