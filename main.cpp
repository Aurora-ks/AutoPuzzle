#include <iostream>
#include <windows.h>
#include <opencv2/opencv.hpp>
#include "ScreenCapture.h"
#include "vision/base.h"
#include "Input.h"

int main() {
    // Set the process to be DPI-aware. This is crucial for correct scaling.
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    // HWND hwnd = FindWindowA("Notepad", nullptr);
    HWND hwnd = (HWND)0x00760900;
    HWND top = (HWND)0x00030420;
    if (!hwnd) {
        std::cout << "can not find top window\n";
        return 1;
    }
    cv::Rect r;
    try {
        sba::ScreenCapture capture(top);
        sba::Input input(top);
        cv::Mat templ = cv::imread("C:\\project\\AutoPuzzle\\images\\0.png");
        capture.start(); // This will block until the capture thread is initialized.

        std::cout << "Capture is running. Waiting for frames..." << std::endl;

        while (true) {
            cv::Mat frame = capture.WaitForNextFrame();
            if (frame.empty()) {
                std::cout << "frame empty" << std::endl;
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                continue;
            }
            cv::imshow("1", frame);
            int key = cv::waitKey(100);
            if (key == 'q') {
                capture.stop();
                break;
            }
            if (key == 's') {
                cv::imwrite("C:\\project\\AutoPuzzle\\images\\0.png",  frame);
                capture.stop();
                break;
            }
            if (key == 'm') {
                auto [rect, val] = sba::Match(frame, templ);
                std::cout << "val:" << val << std::endl;
                // if (val >= 0.9) {
                    r = rect;
                    // int x = rect.x + rect.width / 2;
                    // int y = rect.y + rect.height / 2;
                    // input.MouseClick(x, y, 10);
                    cv::rectangle(frame, rect, cv::Scalar(0, 255, 0), 3);
                    cv::imshow("1", frame);
                    cv::waitKey(0);
                // }
            }
            if (key == 'a') {
                int x = r.x + r.width / 2;
                int y = r.y + r.height / 2;
                POINT p{x, y};
                ClientToScreen(top, &p);
                PostMessage(top, WM_ACTIVATE, WA_ACTIVE, 0);
                SetCursorPos(p.x, p.y);
                input.MouseClickM(x, y);
            }else if (key == 'h') {
                // PostMessage(top, WM_ACTIVATE, WA_ACTIVE, 0);
                int x = r.x + r.width / 2;
                int y = r.y + r.height / 2;
                input.MouseClickH(x, y);
            }
        }

        return 0;

    } catch (const std::exception& ex) {
        std::cerr << "An exception occurred in main: " << ex.what() << std::endl;
        return 1;
    }
}
