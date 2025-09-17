#pragma once

#include <string>
#include <opencv2/opencv.hpp>


// globall debug flag
extern bool g_debug;
// only save when debug is true, rootPath default is current directory
void saveImage(const cv::Mat& image, const std::string& filename, const std::string& rootPath = "");