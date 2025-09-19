cmake_minimum_required (VERSION 3.10)
set(CMAKE_CXX_STANDARD 20)

project ("PSA")
include_directories("src")
add_definitions(-DUNICODE -D_UNICODE)

find_package(OpenCV REQUIRED)
find_package(cppwinrt CONFIG REQUIRED)
#include_directories(${OpenCV_INCLUDE_DIRS})
#add_executable (PSA "main.cpp"  "src/ScreenCapture.cpp" "src/recognition.cpp" "src/utils.cpp" "src/ScreenController.cpp")
add_executable (PSA "main.cpp")

if(MSVC)
    target_compile_options(PSA PRIVATE "/utf-8" "/permissive-")
endif()

target_link_libraries(PSA PRIVATE ${OpenCV_LIBS} Microsoft::CppWinRT windowsapp)