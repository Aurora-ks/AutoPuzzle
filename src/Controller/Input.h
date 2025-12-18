#pragma once

#include <Windows.h>

namespace sba {
enum class MouseButton { Left, Right, Middle };

enum class Key : WORD {
    Esc = VK_ESCAPE,
    Space = VK_SPACE,
    Tab = VK_TAB,
    Enter = VK_RETURN,
    Shift = VK_SHIFT,
    Ctrl = VK_CONTROL,
    Alt = VK_MENU,
    Up = VK_UP,
    Down = VK_DOWN,
    Left = VK_LEFT,
    Right = VK_RIGHT,
    Del = VK_DELETE,
    Backspace = VK_BACK,
    F1 = VK_F1,
    F2 = VK_F2,
    F3 = VK_F3,
    F4 = VK_F4,
    F5 = VK_F5,
    F6 = VK_F6,
    F7 = VK_F7,
    F8 = VK_F8,
    F9 = VK_F9,
    F10 = VK_F10,
    F11 = VK_F11,
    F12 = VK_F12,
};

class Input {
   public:
    explicit Input(HWND hwnd);

    void activate();
    void mouseMove(int x, int y);
    void mouseDown(int x, int y, MouseButton button);
    void mouseUp(int x, int y, MouseButton button);
    void mouseClick(int x, int y, MouseButton button = MouseButton::Left, int clickDurationMs = 200);

    void keyDown(WORD vkCode);
    void keyUp(WORD vkCode);
    void keyPress(Key key, int pressDurationMs = 200);
    void keyPress(char key, int pressDurationMs = 200);

   private:
    static LPARAM buildKeyLParam(WORD vkCode, bool isKeyUp);
    void convertCoordinate(int x, int y, LONG& outAbsX, LONG& outAbsY);  // 将client坐标转换到绝对坐标
    static bool hasMouseMoved(int thresholdPixels = 3, int durationMs = 100);  // 判断在一段时间内鼠标是否移动超过阈值

    HWND window_;
};
}  // namespace sba