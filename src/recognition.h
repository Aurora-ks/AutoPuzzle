#pragma once

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

struct CellInfo {
	enum Status {
		BLOCKED,
		AVAILABLE,
		FILLED
	};
	int id;
	int row;
	int col;
	cv::Point2f center;
	Status status;
	cv::Rect bounds;
};


cv::Mat Image2Hist(const cv::Mat& image);

double ImageHistCompare(const cv::Mat& image, const cv::Mat& hist);

cv::Point TemplateMatch(const cv::Mat& image, const cv::Mat& templateImage);

bool analyzeGrid(const cv::Mat& fullImage, std::vector<CellInfo>& outCells);