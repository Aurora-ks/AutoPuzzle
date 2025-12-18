#include "Input.h"
#include <cctype>
#include <chrono>
#include <cmath>
#include <thread>

namespace sba {
Input::Input(HWND hwnd) {
    if (!IsWindow(hwnd)) throw std::runtime_error("Invalid window handle.");
    window_ = hwnd;
}

LPARAM Input::buildKeyLParam(WORD vkCode, bool isKeyUp) {
    LPARAM lParam = 0;
    UINT scanCode = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC);

    lParam |= 1;  // Repeat count
    lParam |= (scanCode << 16);  // Scan code

    // 检查是否是扩展键 (e.g., R-Ctrl, Insert, Delete, etc.)
    if (vkCode == VK_RCONTROL || vkCode == VK_RMENU || (vkCode >= VK_PRIOR && vkCode <= VK_DELETE)) {
        lParam |= (1 << 24);  // Extended key flag
    }

    if (isKeyUp) {
        lParam |= (1 << 30);  // Previous key state (1 for up)
        lParam |= (1 << 31);  // Transition state (1 for up)
    }

    return lParam;
}

void Input::convertCoordinate(int x, int y, LONG& outAbsX, LONG& outAbsY) {
    // client to screen
    POINT p{x, y};
    ClientToScreen(window_, &p);

    // screen to virtual
    int virtualLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int virtualTop = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int virtualWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int virtualHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    // 计算绝对坐标（0–65535）
    outAbsX = ((p.x - virtualLeft) * 65535) / virtualWidth;
    outAbsY = ((p.y - virtualTop) * 65535) / virtualHeight;
}

bool Input::hasMouseMoved(int thresholdPixels, int durationMs) {
    POINT startPos{}, currentPos{};

    if (!GetCursorPos(&startPos)) {
        // 获取失败时视为未移动，避免误判
        return false;
    }

    if (durationMs > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));
    }

    if (!GetCursorPos(&currentPos)) {
        return false;
    }

    LONG dx = currentPos.x - startPos.x;
    LONG dy = currentPos.y - startPos.y;

    // 当任一方向上的位移绝对值大于等于阈值时，认为“有移动”
    return (std::abs(dx) >= thresholdPixels) || (std::abs(dy) >= thresholdPixels);
}

void Input::activate() {
    PostMessage(window_, WM_ACTIVATE, WA_ACTIVE, 0);
}

void Input::mouseMove(int x, int y) {
    PostMessage(window_, WM_MOUSEMOVE, 0, MAKELPARAM(x, y));
}

void Input::mouseDown(int x, int y, MouseButton button) {
    UINT msg = 0;
    WPARAM wParam = 0;
    switch (button) {
        case MouseButton::Left:
            msg = WM_LBUTTONDOWN;
            wParam = MK_LBUTTON;
            break;
        case MouseButton::Right:
            msg = WM_RBUTTONDOWN;
            wParam = MK_RBUTTON;
            break;
        case MouseButton::Middle:
            msg = WM_MBUTTONDOWN;
            wParam = MK_MBUTTON;
            break;
    }

    PostMessage(window_, msg, wParam, MAKELPARAM(x, y));
}

void Input::mouseUp(int x, int y, MouseButton button) {
    UINT msg = 0;
    switch (button) {
        case MouseButton::Left: msg = WM_LBUTTONUP; break;
        case MouseButton::Right: msg = WM_RBUTTONUP; break;
        case MouseButton::Middle: msg = WM_MBUTTONUP; break;
    }

    PostMessage(window_, msg, 0, MAKELPARAM(x, y));
}

void Input::mouseClick(int x, int y, MouseButton button, int clickDurationMs) {
    // 等待鼠标移动或超时（超时也视为可继续）
    const int timeoutMs = 1000;
    auto start = std::chrono::steady_clock::now();
    while (true) {
        if (!hasMouseMoved()) break;
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
        if (elapsed >= timeoutMs) break;
    }

    // 记录当前鼠标位置
    POINT originalPos{};
    GetCursorPos(&originalPos);

    // 将鼠标移动到目标（将 client 坐标转换为屏幕坐标）
    POINT targetPos{x, y};
    ClientToScreen(window_, &targetPos);
    SetCursorPos(targetPos.x, targetPos.y);

    activate();
    mouseDown(x, y, button);
    if (clickDurationMs > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(clickDurationMs));
    }
    mouseUp(x, y, button);

    // 恢复鼠标到原位置
    SetCursorPos(originalPos.x, originalPos.y);
}

void Input::keyDown(WORD vkCode) {
    PostMessage(window_, WM_KEYDOWN, vkCode, buildKeyLParam(vkCode, false));
}

void Input::keyUp(WORD vkCode) {
    PostMessage(window_, WM_KEYUP, vkCode, buildKeyLParam(vkCode, true));
}

void Input::keyPress(Key key, int pressDurationMs) {
    WORD vkCode = static_cast<WORD>(key);
    keyDown(vkCode);
    if (pressDurationMs > 0) std::this_thread::sleep_for(std::chrono::milliseconds(pressDurationMs));
    keyUp(vkCode);
}

void Input::keyPress(char key, int pressDurationMs) {
    WORD vkCode = 0;
    char c = static_cast<char>(std::tolower(static_cast<unsigned char>(key)));

    if (c >= 'a' && c <= 'z') {
        vkCode = static_cast<WORD>('A' + (c - 'a'));
    } else if (c >= '0' && c <= '9') {
        vkCode = static_cast<WORD>(c);
    } else {
        return;  // 不支持的字符，直接返回
    }

    keyDown(vkCode);
    if (pressDurationMs > 0) std::this_thread::sleep_for(std::chrono::milliseconds(pressDurationMs));
    keyUp(vkCode);
}

}  // namespace sba