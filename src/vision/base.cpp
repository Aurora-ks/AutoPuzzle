#include "base.h"
#include <tuple>

namespace sba {
using enum MatchMode;

std::tuple<cv::Rect, double> Match(const cv::Mat &frame, const cv::Mat &TemplateImg, const cv::Mat &mask, const MatchMode PreprocessMode) {
    cv::Mat FrameProc, TemplateProc;

    // Pre-process
    switch (PreprocessMode) {
        case NORMAL:
            FrameProc = frame;
            TemplateProc = TemplateImg;
            break;
        case GRAY:
            cv::cvtColor(frame, FrameProc, cv::COLOR_BGR2GRAY);
            cv::cvtColor(TemplateImg, TemplateProc, cv::COLOR_BGR2GRAY);
            break;
        case HSV:
            cv::cvtColor(frame, FrameProc, cv::COLOR_BGR2HSV);
            cv::cvtColor(TemplateImg, TemplateProc, cv::COLOR_BGR2HSV);
            break;
    }

    cv::Mat result;
    cv::matchTemplate(FrameProc, TemplateProc, result, cv::TM_CCOEFF_NORMED, mask);

    // Find the best match
    double MaxVal;
    cv::Point MaxLoc;
    cv::minMaxLoc(result, nullptr, &MaxVal, nullptr, &MaxLoc);

    // For TM_CCOEFF_NORMED, the best match is at the max value location.
    cv::Rect rect(MaxLoc, cv::Point(MaxLoc.x + TemplateImg.cols, MaxLoc.y + TemplateImg.rows));

    return std::make_tuple(rect, MaxVal);
}

cv::Mat GetMask(const cv::Mat &img, const int threshold) {
    cv::Mat gray, result;
    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    cv::threshold(gray, result, threshold, 255, cv::THRESH_BINARY);
    return result;
}
} // namespace sba