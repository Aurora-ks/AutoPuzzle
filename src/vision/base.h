#pragma once

#include <opencv2/opencv.hpp>
#include <random>
#include <tuple>

namespace sba {

struct Rect {
    // client coordinates
    int x, y;  // left top point
    int width, height;
    double confidence;

    Rect(int x, int y, int width, int height, double confidence = 0) : x(x), y(y), width(width), height(height), confidence(confidence) {}

    std::tuple<int, int> center() { return std::make_tuple(x + (width >> 1), y + (height >> 1)); }

    // 在缩放后的区域内随机返回一个坐标点
    // scale: 缩放比例 (0.0-1.0)
    std::tuple<int, int> randomPoint(double scale = 0.8) {
        static std::random_device rd;
        static std::mt19937 gen(rd());

        int scaledWidth = static_cast<int>(width * scale);
        int scaledHeight = static_cast<int>(height * scale);
        int offsetX = (width - scaledWidth) / 2;
        int offsetY = (height - scaledHeight) / 2;

        std::uniform_int_distribution<> distX(0, scaledWidth > 0 ? scaledWidth - 1 : 0);
        std::uniform_int_distribution<> distY(0, scaledHeight > 0 ? scaledHeight - 1 : 0);

        return std::make_tuple(x + offsetX + distX(gen), y + offsetY + distY(gen));
    }
};
// pre-process mode
enum class MatchMode {
    NORMAL,
    GRAY,
    HSV,
};

Rect Match(const cv::Mat& frame, const cv::Mat& TemplateImg, const cv::Mat& mask = cv::Mat(), MatchMode PreprocessMode = MatchMode::NORMAL);
cv::Mat GetMask(const cv::Mat& img, int threshold);
}  // namespace sba