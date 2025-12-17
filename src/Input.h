#pragma once

#include <Windows.h>
#include <string>

namespace sba {
enum class MouseButton { Left, Right, Middle };

class Input {
   public:
    explicit Input(HWND hwnd);

    inline static constexpr const char* kKeyEsc = "esc";
    inline static constexpr const char* kKeySpace = "space";
    inline static constexpr const char* kKeyTab = "tab";
    inline static constexpr const char* kKeyEnter = "enter";
    inline static constexpr const char* kKeyShift = "shift";
    inline static constexpr const char* kKeyCtrl = "ctrl";
    inline static constexpr const char* kKeyAlt = "alt";
    inline static constexpr const char* kKeyUp = "up";
    inline static constexpr const char* kKeyDown = "down";
    inline static constexpr const char* kKeyLeft = "left";
    inline static constexpr const char* kKeyRight = "right";
    inline static constexpr const char* kKeyDel = "del";
    inline static constexpr const char* kKeyBackspace = "backspace";

    void activate();
    void mouseMove(int x, int y);
    void mouseDown(int x, int y, MouseButton button);
    void mouseUp(int x, int y, MouseButton button);
    void mouseClick(int x, int y, MouseButton button = MouseButton::Left, int clickDurationMs = 200);

    void keyDown(WORD vkCode);
    void keyUp(WORD vkCode);
    void keyPress(const std::string& key, int pressDurationMs = 200);

   private:
    static LPARAM buildKeyLParam(WORD vkCode, bool isKeyUp);
    void convertCoordinate(int x, int y, LONG& outAbsX, LONG& outAbsY);  // 将client坐标转换到绝对坐标
    static bool hasMouseMoved(int thresholdPixels = 3, int durationMs = 100);  // 判断在一段时间内鼠标是否移动超过阈值
    static WORD parseKey(const std::string& key);  // 将字符串转换为虚拟键码

    HWND window_;
};
}  // namespace sba