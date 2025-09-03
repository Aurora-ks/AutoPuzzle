#include <iostream>
#include <opencv2/opencv.hpp>

using namespace std;

int main() {

    cv::Mat fullImage = cv::imread("C:\\Project\\ProjectSnow\\1.jpg");

    if (fullImage.empty()) {
        cout << "cannot load image" << endl;
        return -1;
    }

    return 0;
}