#pragma once

#include <Windows.h>
#include <random>
#include <chrono>
#include <thread>

namespace sba {
enum class MouseButton {
    Left,
    Right,
    Middle
};

class Input {
public:
    explicit Input(HWND hwnd);

    void MouseMoveH(int x, int y);
    void MouseMoveM(int x, int y);

    void MouseDownM(int x, int y);
    void MouseUpM(int x, int y);
    void MouseUpH(int x, int y);
    void MouseDownH(int x, int y);

    void MouseClickM(int x, int y, int clickDurationMs = 50);
    void MouseClickH(int x, int y, int clickDurationMs = 50);

    // --- 键盘操作 ---

    /**
     * @brief 模拟在后台窗口按下指定按键
     * @param vkCode 虚拟键码 (e.g., 'W', VK_SPACE)
     */
    void KeyDownM(WORD vkCode);

    /**
     * @brief 模拟在后台窗口释放指定按键
     * @param vkCode 虚拟键码 (e.g., 'W', VK_SPACE)
     */
    void KeyUpM(WORD vkCode);

    /**
     * @brief 模拟在后台窗口的一次完整按键（单击）
     * @param vkCode 虚拟键码
     * @param pressDurationMs 按下和抬起之间的模拟延迟（毫秒）
     */
    void KeyDownH(WORD vkCode);
    void KeyUpH(WORD vkCode);
    void KeyPressM(WORD vkCode, int pressDurationMs = 50);
    void KeyPressH(WORD vkCode, int pressDurationMs = 50);

private:
    /**
     * @brief 为键盘消息构造 lParam 参数
     * @param vkCode 虚拟键码
     * @param isKeyUp 标志是按下(false)还是抬起(true)
     * @return 构造好的 lParam 值
     */
    static LPARAM BuildKeyLParam_(WORD vkCode, bool isKeyUp);
    inline void CheckWindow_() const;
    void ConvertCoordinate(int x, int y, LONG &outAbsX, LONG &outAbsY); // 将client坐标转换到绝对坐标

    HWND window_;
};
}// namespace sba