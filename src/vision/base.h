#pragma once

#include <opencv2/opencv.hpp>
#include <tuple>

namespace sba {
    // pre-process mode
    enum class MatchMode {
        NORMAL,
        GRAY,
        HSV,
    };

    std::tuple<cv::Rect, double> Match(const cv::Mat& frame, const cv::Mat& TemplateImg);
} // sba