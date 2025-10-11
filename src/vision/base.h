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

    std::tuple<cv::Rect, double> Match(const cv::Mat &frame, const cv::Mat &TemplateImg, const cv::Mat &mask = cv::Mat(), MatchMode PreprocessMode = MatchMode::NORMAL);
    cv::Mat GetMask(const cv::Mat &img, int threshold);
} // namespace sba