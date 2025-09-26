#include "base.h"
#include <tuple>

namespace sba {
    using enum MatchMode;

    std::tuple<cv::Rect, double> Match(const cv::Mat &frame, const cv::Mat &TemplateImg, const MatchMode PreprocessMode = NORMAL) {
        cv::Mat FrameProc, TemplateProc;

        // Pre-process
        switch (PreprocessMode) {
            case NORMAL: break;
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
        cv::matchTemplate(FrameProc, TemplateProc, result, cv::TM_CCOEFF_NORMED);

        // Find the best match
        double MaxVal;
        cv::Point MaxLoc;
        cv::minMaxLoc(result, nullptr, &MaxVal, nullptr, &MaxLoc, cv::Mat());

        // For TM_CCOEFF_NORMED, the best match is at the max value location.
        cv::Rect rect(MaxLoc, cv::Point(MaxLoc.x + TemplateImg.cols, MaxLoc.y + TemplateImg.rows));

        return std::make_tuple(rect, MaxVal);
    }
} // sba