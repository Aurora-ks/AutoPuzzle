#include "recognition.h"
#include "utils.h"
#include <iostream>

using namespace cv;
using namespace std;

static void saveImages(const Mat& image, const string& filename) {
	saveImage(image, filename, "grid");
}

Mat Image2Hist(const Mat& image) {
	Mat imageHSV;
	cvtColor(image, imageHSV, COLOR_BGR2HSV);

	int histSize[] = { 50, 60 };
	
	float hRanges[] = { 0, 180 };
	float sRanges[] = { 0, 256 };
	const float* ranges[] = { hRanges, sRanges };
	int channels[] = { 0, 1 };

	Mat hist;
	calcHist(&image, 1, channels, Mat(), hist, 2, histSize, ranges);
	normalize(hist, hist, 1.0, 0.0, NORM_L1);

	return hist;
}

double ImageHistCompare(const Mat& image, const Mat& hist) {
	return compareHist(Image2Hist(image), hist, HISTCMP_BHATTACHARYYA);
}

Point TemplateMatch(const Mat& image, const Mat& templateImage) {
	Mat imageHSV, templateHSV;
	cvtColor(image, imageHSV, COLOR_BGR2HSV);
    cvtColor(templateImage, templateHSV, COLOR_BGR2HSV);
	Mat result;
	matchTemplate(image, templateImage, result, TM_CCOEFF_NORMED);
	double min, max;
	Point minLoc, maxLoc;
    minMaxLoc(result, &min, &max, &minLoc, &maxLoc);
	return maxLoc;
}

bool analyzeGrid(const Mat& fullImage, vector<CellInfo>& outCells) {
	if (fullImage.empty()) {
		cerr << "Invalid image" << endl;
		return false;
	}

	saveImages(fullImage, "01_original.jpg");

	// 灰度转换
	Mat gray, blurred;
	cvtColor(fullImage, gray, COLOR_BGR2GRAY);
	saveImages(gray, "02_gray.jpg");

	//GaussianBlur(gray, blurred, Size(5, 5), 0);
	//saveImages(blurred, "03_blurred.jpg");

	// 边缘检测
	Mat edges;
	Canny(gray, edges, 50, 150);
	saveImages(edges, "04_edges.jpg");

	// 形态学操作连接断开的线条
	Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3));
	Mat morphed;
	morphologyEx(edges, morphed, MORPH_CLOSE, kernel);
	saveImages(morphed, "05_morphed.jpg");
	
	// 寻找网格区域（最大的矩形轮廓）
	vector<vector<Point>> contours;
	findContours(morphed, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

	// 绘制检测到的所有轮廓
	if (g_debug) {
		Mat contoursAllVis = fullImage.clone();
		for (size_t i = 0; i < contours.size(); ++i) {
			Scalar color = Scalar((37 * (int)i) % 256, (97 * (int)i) % 256, (151 * (int)i) % 256);
			vector<vector<Point>> single; single.push_back(contours[i]);
			drawContours(contoursAllVis, single, -1, color, 2);

			Moments mu = moments(contours[i]);
			Point2f c;
			if (fabs(mu.m00) > 1e-3) {
				c = Point2f(static_cast<float>(mu.m10 / mu.m00), static_cast<float>(mu.m01 / mu.m00));
			}
			else {
				Rect br = boundingRect(contours[i]);
				c = Point2f(static_cast<float>(br.x + br.width / 2.0f), static_cast<float>(br.y + br.height / 2.0f));
			}
			putText(contoursAllVis, to_string(i), c + Point2f(1, 1), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(0, 0, 0), 2);
			putText(contoursAllVis, to_string(i), c, FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 255, 255), 1);
		}
		saveImages(contoursAllVis, "06_contours_all.jpg");
	}

	// 找到最大的矩形轮廓
	double maxArea = 0;
	vector<Point> gridContour;
	for (const auto& contour : contours) {
		double area = contourArea(contour);
		if (area > maxArea) {
			maxArea = area;
			gridContour = contour;
		}
	}

	if (gridContour.empty()) {
		cerr << "No grid found" << endl;
		return false;
	}

	if (!g_debug) {
		Mat gridContourVis = fullImage.clone();
		vector<vector<Point>> gridOnly; gridOnly.push_back(gridContour);
		drawContours(gridContourVis, gridOnly, -1, Scalar(0, 0, 255), 3);
		saveImages(gridContourVis, "06_grid_contour.jpg");
	}

	// 获取网格边界矩形
	Rect gridRect = boundingRect(gridContour);

	if (g_debug) {
		Mat bboxVis = fullImage.clone();
		rectangle(bboxVis, gridRect, Scalar(255, 0, 0), 2);
		saveImages(bboxVis, "07_grid_bbox.jpg");
	}

	Mat gridImage = fullImage(gridRect);
	saveImages(gridImage, "07_grid_region.jpg");

	// 计算单元格尺寸
	int cellWidth = gridImage.cols / 6;  // 6列
	int cellHeight = gridImage.rows / 5; // 5行

	// 分割单元格并识别状态
	Mat cellAnalysisImage = gridImage.clone();
	for (int row = 0; row < 5; row++) {
		for (int col = 0; col < 6; col++) {
			int cellId = row * 6 + col + 1;

			int x = col * cellWidth;
			int y = row * cellHeight;
			Rect cellRect(x, y, cellWidth, cellHeight);
			Mat cellRegion = gridImage(cellRect);

			Point2f center(static_cast<float>(x + cellWidth / 2.0f), static_cast<float>(y + cellHeight / 2.0f));

			Mat cellGray;
			cvtColor(cellRegion, cellGray, COLOR_BGR2GRAY);
			Scalar meanBrightness = mean(cellGray);
			double brightness = meanBrightness[0];

			CellInfo::Status status;
			Scalar color;
			if (brightness > 200) {
				status = CellInfo::Status::BLOCKED;
				color = Scalar(0, 0, 255);
			}
			else {
				status = CellInfo::Status::AVAILABLE;
				color = Scalar(0, 255, 0);
			}

			CellInfo cell;
			cell.id = cellId;
			cell.row = row + 1;
			cell.col = col + 1;
			cell.center = Point2f(static_cast<float>(gridRect.x) + center.x, static_cast<float>(gridRect.y) + center.y);
			cell.status = status;
			cell.bounds = Rect(gridRect.x + x, gridRect.y + y, cellWidth, cellHeight);
			outCells.push_back(cell);
			
			if (g_debug) {
				rectangle(cellAnalysisImage, cellRect, color, 2);
				putText(cellAnalysisImage, to_string(cellId), Point(x + 5, y + 20), FONT_HERSHEY_SIMPLEX, 0.5, color, 1);
				putText(cellAnalysisImage, status == CellInfo::Status::BLOCKED ? "blocked" : "avaliable", Point(x + 5, y + cellHeight - 5), FONT_HERSHEY_SIMPLEX, 0.4, color, 1);
				saveImages(cellRegion, string("08_cell_") + to_string(cellId) + ".jpg");
			}
		}
	}

	saveImages(cellAnalysisImage, "8_cell_analysis.jpg");

	// 原图可视化
	if (g_debug) {
		Mat outFinalResult = fullImage.clone();
		for (const auto& cell : outCells) {
			Scalar color = (cell.status == CellInfo::Status::BLOCKED) ? Scalar(0, 0, 255) : Scalar(0, 255, 0);
			Rect absoluteBounds = cell.bounds;
			Point2f absoluteCenter = cell.center;
			rectangle(outFinalResult, absoluteBounds, color, 2);
			putText(outFinalResult, to_string(cell.id), Point(absoluteBounds.x + 5, absoluteBounds.y + 20), FONT_HERSHEY_SIMPLEX, 0.6, color, 2);
			circle(outFinalResult, absoluteCenter, 3, color, -1);
			circle(outFinalResult, absoluteCenter, 5, Scalar(255, 255, 255), 1);
		}
		saveImages(outFinalResult, "9_final_result.jpg");
	}

	return true;
}


