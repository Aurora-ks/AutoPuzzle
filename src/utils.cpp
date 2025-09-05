#include "utils.h"
#include <iostream>
#include <filesystem>

using namespace std;
using namespace std::filesystem;

bool g_debug = false;

void saveImage(const cv::Mat& image, const std::string& filename, const std::string& rootPath) {
	if (!g_debug || image.empty()) return;

	path basePath = path("./logs/images");
	path root = path(rootPath);
	path full = (basePath / root / filename).lexically_normal();

	error_code ec;
	create_directories(full.parent_path(), ec);
	if (ec) {
		cerr << "failed to create directories: " << full.parent_path().string() << ", error: " << ec.message() << endl;
	}

	bool ok = cv::imwrite(full.string(), image);
	if (ok) {
		cout << "save image: " << full.string() << endl;
	}
	else {
		cerr << "failed to save image: " << full.string() << endl;
	}
}