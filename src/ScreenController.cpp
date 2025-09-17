#include "ScreenController.h"

namespace psa {
    void setWindowTo1920x1080(HWND handle) {
        if (handle) {
            // Define the desired window style for a standard windowed mode.
            DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;

            // Apply the new window style.
            SetWindowLong(handle, GWL_STYLE, style);

            RECT rect = {0, 0, 1920, 1080};

            // Adjust the window rectangle to account for the new style's borders, title bar, etc.,
            // so that the client area becomes 1920x1080.
            AdjustWindowRect(&rect, style, FALSE);

            int width = rect.right - rect.left;
            int height = rect.bottom - rect.top;

            // Set the window size and position.
            // SWP_NOMOVE keeps the window in its current location.
            // SWP_NOZORDER keeps the window in its current Z-order.
            // SWP_FRAMECHANGED forces the window to repaint with the new style.
            SetWindowPos(handle, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
        }
    }
}

