#include "Input.h"
#include <iostream>

namespace sba {
Input::Input(HWND hwnd){
    if (!IsWindow(hwnd)) throw std::runtime_error("Invalid window handle.");
    window_ = hwnd;
}

LPARAM Input::BuildKeyLParam_(WORD vkCode, bool isKeyUp) {
    LPARAM lParam = 0;
    UINT scanCode = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC);

    lParam |= 1; // Repeat count
    lParam |= (scanCode << 16); // Scan code

    // 检查是否是扩展键 (e.g., R-Ctrl, Insert, Delete, etc.)
    if (vkCode == VK_RCONTROL || vkCode == VK_RMENU ||
        (vkCode >= VK_PRIOR && vkCode <= VK_DELETE)) {
        lParam |= (1 << 24); // Extended key flag
    }

    if (isKeyUp) {
        lParam |= (1 << 30); // Previous key state (1 for up)
        lParam |= (1 << 31); // Transition state (1 for up)
    }

    return lParam;
}

void Input::CheckWindow_() const {
    if (!IsWindow(window_)) throw std::runtime_error("Invalid window handle.");
}

void Input::ConvertCoordinate(int x, int y, LONG &outAbsX, LONG &outAbsY) {
    // client to screen
    CheckWindow_();
    POINT p{x, y};
    ClientToScreen(window_, &p);

    // screen to virtual
    int virtualLeft   = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int virtualTop    = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int virtualWidth  = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int virtualHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    // 计算绝对坐标（0–65535）
    outAbsX = ((p.x - virtualLeft) * 65535) / virtualWidth;
    outAbsY = ((p.y - virtualTop) * 65535) / virtualHeight;
}

void Input::MouseMoveH(int x, int y) {
    LONG absX, absY;
    ConvertCoordinate(x, y, absX, absY);

    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dx = absX;
    input.mi.dy = absY;
    input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | MOUSEEVENTF_VIRTUALDESK;

    SendInput(1, &input, sizeof(INPUT));
}

void Input::MouseMoveM(int x, int y) {
    CheckWindow_();
    PostMessage(window_, WM_MOUSEMOVE, 0, MAKELPARAM(x, y));
}


void Input::MouseDownM(int x, int y) {
    CheckWindow_();
    PostMessage(window_, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(x, y));
}

void Input::MouseUpM(int x, int y) {
    CheckWindow_();
    PostMessage(window_, WM_LBUTTONUP, 0, MAKELPARAM(x, y));
}

void Input::MouseDownH(int x, int y) {
    MouseMoveH(x, y);
    INPUT input{0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    SendInput(1, &input, sizeof(INPUT));
}

void Input::MouseUpH(int x, int y) {
    MouseMoveH(x, y);
    INPUT input{0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(1, &input, sizeof(INPUT));
}

void Input::MouseClickM(int x, int y, int clickDurationMs) {
    MouseMoveM(x, y);
    MouseDownM(x, y);
    if (clickDurationMs > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(clickDurationMs));
    }
    MouseUpM(x, y);
}

void Input::MouseClickH(int x, int y, int clickDurationMs) {
    MouseDownH(x, y);
    if (clickDurationMs > 0) std::this_thread::sleep_for(std::chrono::microseconds(clickDurationMs));
    MouseUpH(x, y);
}

void Input::KeyDownM(WORD vkCode) {
    CheckWindow_();
    PostMessage(window_, WM_KEYDOWN, vkCode, BuildKeyLParam_(vkCode, false));
}

void Input::KeyUpM(WORD vkCode) {
    CheckWindow_();
    PostMessage(window_, WM_KEYUP, vkCode, BuildKeyLParam_(vkCode, true));
}

void Input::KeyDownH(WORD vkCode) {
    INPUT input{0};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vkCode;
    input.ki.dwFlags = 0;
    SendInput(1, &input, sizeof(input));
}

void Input::KeyUpH(WORD vkCode) {
    INPUT input{0};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vkCode;
    input.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(input));
}

void Input::KeyPressM(WORD vkCode, int pressDurationMs) {
    KeyDownM(vkCode);
    if (pressDurationMs > 0) std::this_thread::sleep_for(std::chrono::milliseconds(pressDurationMs));
    KeyUpM(vkCode);
}

void Input::KeyPressH(WORD vkCode, int pressDurationMs) {
    KeyDownH(vkCode);
    if (pressDurationMs > 0) std::this_thread::sleep_for(std::chrono::milliseconds(pressDurationMs));
    KeyUpH(vkCode);
}

} // namespace sba