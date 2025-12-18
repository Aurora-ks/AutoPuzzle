cmake_minimum_required (VERSION 3.10)
set(CMAKE_CXX_STANDARD 20)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "choose the build type" FORCE)
endif ()

project ("SnowbreakAssistant")

find_package(OpenCV REQUIRED)
find_package(cppwinrt CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

add_executable (${PROJECT_NAME} "main.cpp" "src/vision/Base.cpp" "src/Controller/Capture.cpp" "src/Controller/Input.cpp")

target_include_directories(${PROJECT_NAME} PRIVATE "src")

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE "/utf-8" "/permissive-")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE SBA_DEBUG)
endif ()

target_link_libraries(${PROJECT_NAME} PRIVATE opencv_core opencv_imgproc opencv_highgui opencv_imgcodecs Microsoft::CppWinRT windowsapp yaml-cpp::yaml-cpp)