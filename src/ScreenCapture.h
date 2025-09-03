#ifndef SCREEN_CAPTURE_H
#define SCREEN_CAPTURE_H

#include <opencv2/opencv.hpp>
#include <string>

/**
 * @brief 屏幕捕获模块
 * 负责捕获游戏窗口的内容并转换为OpenCV的Mat格式
 * 
 * 该模块提供以下功能：
 * 1. 根据窗口名称捕获指定窗口
 * 2. 根据窗口句柄捕获窗口内容
 * 3. 捕获指定屏幕区域
 * 
 * 所有捕获操作均返回OpenCV的Mat格式图像，便于后续图像处理
 */
class ScreenCapture {
public:
    /**
     * @brief 捕获指定窗口的内容
     * @param windowName 窗口名称
     * @param frame 输出的帧图像
     * @return 是否成功捕获
     * 
     * 该函数会先尝试通过窗口标题查找窗口句柄，
     * 如果失败则尝试通过窗口类名查找。
     */
    static bool CaptureGameWindow(const std::string& windowName, cv::Mat& frame);

    /**
     * @brief 捕获指定区域的屏幕内容
     * @param x 截图区域左上角x坐标
     * @param y 截图区域左上角y坐标
     * @param width 截图区域宽度
     * @param height 截图区域高度
     * @param frame 输出的帧图像
     * @return 是否成功捕获
     * 
     * 直接通过指定坐标和尺寸捕获屏幕区域，
     * 适用于已知具体坐标的情况。
     */
    static bool CaptureScreenRegion(int x, int y, int width, int height, cv::Mat& frame);
    
    /**
     * @brief 根据窗口句柄完整捕获窗口内容（包括子窗口）
     * @param hwnd 窗口句柄
     * @param frame 输出的帧图像
     * @return 是否成功捕获
     * 
     * 使用PrintWindow API捕获完整的窗口内容，包括所有子窗口和自定义绘制的内容。
     * 如果PrintWindow不可用或失败，将回退到BitBlt方法。
     */
    static bool CaptureWindowComplete(void* hwnd, cv::Mat& frame);

    /**
     * @brief 获取屏幕尺寸
     * @param width 屏幕宽度（输出参数）
     * @param height 屏幕高度（输出参数）
     * @return 是否成功获取
     */
    static bool GetScreenSize(int& width, int& height);

    /**
     * @brief 测试函数：将cv::Mat保存为图片文件
     * @param frame 要保存的图像
     * @param filename 保存的文件名
     * @return 是否保存成功
     */
    static bool SaveFrameAsImage(const cv::Mat& frame, const std::string& filename);
};

#endif // SCREEN_CAPTURE_H