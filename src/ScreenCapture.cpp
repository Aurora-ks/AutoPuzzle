#include "ScreenCapture.h"
#include "utils.h"
#include <windows.h>
#include <iostream>

/**
 * @brief 捕获指定窗口的内容
 * @param windowName 窗口名称
 * @param frame 输出的帧图像
 * @return 是否成功捕获
 */
bool CaptureGameWindow(const std::string& windowName, cv::Mat& frame) {
    // 查找窗口句柄
    HWND hwnd = FindWindow(nullptr, windowName.c_str());
    if (hwnd == nullptr) {
        hwnd = FindWindow(windowName.c_str(), nullptr);
    }
    
    if (hwnd == nullptr) {
        std::cerr << "Error: Cannot find window: " << windowName << std::endl;
        return false;
    }

    return CaptureWindowComplete(hwnd, frame);
}

/**
 * @brief 根据窗口句柄完整捕获窗口内容（包括子窗口）
 * @param hwnd 窗口句柄
 * @param frame 输出的帧图像
 * @return 是否成功捕获
 * 
 * 使用PrintWindow API捕获完整的窗口内容，包括所有子窗口和自定义绘制的内容。
 * 如果PrintWindow不可用或失败，将回退到BitBlt方法。
 */
bool CaptureWindowComplete(void* hwnd, cv::Mat& frame) {
    HWND hWnd = reinterpret_cast<HWND>(hwnd);
    
    // 检查窗口句柄有效性
    if (!IsWindow(hWnd)) {
        std::cerr << "Error: Invalid window handle" << std::endl;
        return false;
    }
    
    // 获取窗口尺寸
    RECT windowRect;
    if (!GetWindowRect(hWnd, &windowRect)) {
        std::cerr << "Error: Cannot get window rect" << std::endl;
        return false;
    }
    
    // 获取DPI缩放因子
    UINT dpiX = GetDpiForWindow(hWnd);
    UINT dpiY = GetDpiForWindow(hWnd);
    float scale = static_cast<float>(dpiX) / 96.0f;

    // 应用DPI缩放调整窗口尺寸
    int width = static_cast<int>((windowRect.right - windowRect.left) * scale);
    int height = static_cast<int>((windowRect.bottom - windowRect.top) * scale);

    if (width <= 0 || height <= 0) {
        std::cerr << "Error: Invalid window size after DPI scaling: " << width << "x" << height << std::endl;
        return false;
    }
    
    // 创建设备上下文
    HDC hScreenDC = GetDC(hWnd);
    if (hScreenDC == nullptr) {
        std::cerr << "Error: Cannot get screen device context" << std::endl;
        return false;
    }
    
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
    if (hMemoryDC == nullptr) {
        ReleaseDC(nullptr, hScreenDC);
        std::cerr << "Error: Cannot create memory device context" << std::endl;
        return false;
    }
    
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    if (hBitmap == nullptr) {
        DeleteDC(hMemoryDC);
        ReleaseDC(nullptr, hScreenDC);
        std::cerr << "Error: Cannot create compatible bitmap" << std::endl;
        return false;
    }
    
    HGDIOBJ hOldBitmap = SelectObject(hMemoryDC, hBitmap);
    if (hOldBitmap == nullptr || hOldBitmap == HGDI_ERROR) {
        DeleteObject(hBitmap);
        DeleteDC(hMemoryDC);
        ReleaseDC(nullptr, hScreenDC);
        std::cerr << "Error: Cannot select bitmap into memory DC" << std::endl;
        return false;
    }

    // 使用PrintWindow捕获完整窗口（包括子窗口）
    BOOL printResult = PrintWindow(hWnd, hMemoryDC, PW_CLIENTONLY);
    
    // 如果PrintWindow失败，回退到BitBlt方法
    if (!printResult) {
        std::cerr << "Error: PrintWindow failed, trying BitBlt" << std::endl;
        HDC hWindowDC = GetWindowDC(hWnd);
        if (hWindowDC != nullptr) {
            BitBlt(hMemoryDC, 0, 0, width, height, hWindowDC, 0, 0, SRCCOPY);
            ReleaseDC(hWnd, hWindowDC);
        } else {
            // 如果无法获取窗口DC，清理资源并返回错误
            SelectObject(hMemoryDC, hOldBitmap);
            DeleteObject(hBitmap);
            DeleteDC(hMemoryDC);
            ReleaseDC(nullptr, hScreenDC);
            std::cerr << "Error: Cannot get window device context" << std::endl;
            return false;
        }
    }
    
    // 第一次查询：获取实际位深（biBitCount）等信息
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biCompression = BI_RGB;
    if (!GetDIBits(hScreenDC, hBitmap, 0, 0, nullptr, &bmi, DIB_RGB_COLORS)) {
        SelectObject(hMemoryDC, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hMemoryDC);
        ReleaseDC(nullptr, hScreenDC);
        std::cerr << "Error: Query GetDIBits failed" << std::endl;
        return false;
    }

    WORD bitCount = bmi.bmiHeader.biBitCount;
    if (bitCount == 0) {
        bitCount = 32; // 兜底为 32 位
    }
    int channels = bitCount == 1 ? 1 : static_cast<int>(bitCount) / 8; // 常见：8/24/32
    if (channels != 1 && channels != 3 && channels != 4) {
        channels = 4; // 非预期位深时兜底到 4 通道
    }

    // 设置为 top-down 并使用实际位深
    ZeroMemory(&bmi, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = static_cast<WORD>(channels * 8);
    bmi.bmiHeader.biCompression = BI_RGB;

    cv::Mat rawImage;
    if (channels == 4) {
        rawImage.create(height, width, CV_8UC4);
    } else if (channels == 3) {
        rawImage.create(height, width, CV_8UC3);
    } else {
        rawImage.create(height, width, CV_8UC1);
    }

    if (!GetDIBits(hScreenDC, hBitmap, 0, height, rawImage.data, &bmi, DIB_RGB_COLORS)) {
        SelectObject(hMemoryDC, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hMemoryDC);
        ReleaseDC(nullptr, hScreenDC);
        std::cerr << "Error: Cannot get bitmap data via GetDIBits" << std::endl;
        return false;
    }

    // 统一输出为 BGR
    if (channels == 4) {
        cv::cvtColor(rawImage, frame, cv::COLOR_BGRA2BGR);
    } else if (channels == 3) {
        frame = rawImage.clone();
    } else {
        cv::cvtColor(rawImage, frame, cv::COLOR_GRAY2BGR);
    }
    saveImage(frame, "screen_shot.png", "./screen");

    // 清理资源
    SelectObject(hMemoryDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(nullptr, hScreenDC);
    
    return true;
}

/**
 * @brief 捕获指定区域的屏幕内容
 * @param x 截图区域左上角x坐标
 * @param y 截图区域左上角y坐标
 * @param width 截图区域宽度
 * @param height 截图区域高度
 * @param frame 输出的帧图像
 * @return 是否成功捕获
 */
bool CaptureScreenRegion(int x, int y, int width, int height, cv::Mat& frame) {
    // 参数验证
    if (width <= 0 || height <= 0) {
        std::cerr << "Error: Invalid capture region size: " << width << "x" << height << std::endl;
        return false;
    }
    
    // 获取屏幕设备上下文
    HDC hScreenDC = GetDC(nullptr);
    if (hScreenDC == nullptr) {
        std::cerr << "Error: Cannot get screen device context" << std::endl;
        return false;
    }
    
    // 创建内存设备上下文
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
    if (hMemoryDC == nullptr) {
        ReleaseDC(nullptr, hScreenDC);
        std::cerr << "Error: Cannot create memory device context" << std::endl;
        return false;
    }
    
    // 创建位图
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    if (hBitmap == nullptr) {
        DeleteDC(hMemoryDC);
        ReleaseDC(nullptr, hScreenDC);
        std::cerr << "Error: Cannot create compatible bitmap" << std::endl;
        return false;
    }
    
    // 选择位图到内存DC
    HGDIOBJ hOldBitmap = SelectObject(hMemoryDC, hBitmap);
    
    // 将屏幕内容复制到内存DC
    if (!BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, x, y, SRCCOPY)) {
        SelectObject(hMemoryDC, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hMemoryDC);
        ReleaseDC(nullptr, hScreenDC);
        std::cerr << "Error: Cannot copy screen content to memory" << std::endl;
        return false;
    }
    
    // 获取位图信息
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // 负值表示top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    
    // 分配内存存储像素数据
    cv::Mat rawImage(height, width, CV_8UC4);
    if (!GetDIBits(hScreenDC, hBitmap, 0, height, rawImage.data, &bmi, DIB_RGB_COLORS)) {
        SelectObject(hMemoryDC, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(hMemoryDC);
        ReleaseDC(nullptr, hScreenDC);
        std::cerr << "Error: Cannot get bitmap data" << std::endl;
        return false;
    }
    
    // 转换为OpenCV格式 (BGRA to BGR)
    cv::cvtColor(rawImage, frame, cv::COLOR_BGRA2BGR);
    
    // 清理资源
    SelectObject(hMemoryDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(nullptr, hScreenDC);
    
    return true;
}

/**
 * @brief 获取屏幕尺寸
 * @param width 屏幕宽度（输出参数）
 * @param height 屏幕高度（输出参数）
 * @return 是否成功获取
 */
bool GetScreenSize(int& width, int& height) {
    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    
    if (width <= 0 || height <= 0) {
        std::cerr << "Error: Cannot get screen size" << std::endl;
        return false;
    }
    
    return true;
}