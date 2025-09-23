#include <iostream>
#include <windows.h>
#include <opencv2/opencv.hpp>
#include "src/ScreenCapture.h"

int main() {
    // Set the process to be DPI-aware. This is crucial for correct scaling.
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    try {
        sba::ScreenCapture screenCapture;
        screenCapture.start();

        while (true) {
            cv::Mat displayFrame = screenCapture.GetLatestFrame();
            if (!displayFrame.empty())
                cv::imshow("Screen Capture", displayFrame);

            if (cv::waitKey(50) == 'q') {
                screenCapture.stop();
                break;
            }
        }

        cv::destroyAllWindows();

    } catch (const std::exception& ex) {
        std::cerr << "An exception occurred: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}