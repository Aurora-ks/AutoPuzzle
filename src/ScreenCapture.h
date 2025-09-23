#pragma once

#include <windows.h>
#include <mutex>
#include <opencv2/opencv.hpp>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <d3d11.h>
#include <dxgi1_2.h>

namespace sba {
class ScreenCapture {
public:
    explicit ScreenCapture();
    ~ScreenCapture();

    void start();
    void stop();
    cv::Mat GetLatestFrame();

private:
    void OnFrameArrived_(
        const winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool& sender,
        const winrt::Windows::Foundation::IInspectable& args);
    
    void Cleanup_();

    static winrt::com_ptr<ID3D11Device> CreateD3DDevice_();
    static winrt::Windows::Graphics::Capture::GraphicsCaptureItem CreateCaptureItemForWindow_(HWND hwnd);

    // Direct3D resources
    winrt::com_ptr<ID3D11Device> d3dDevice_; // The core Direct3D 11 device.
    winrt::com_ptr<ID3D11DeviceContext> d3dContext_; // The immediate context for the D3D device.
    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice direct3DDevice_{nullptr}; // WinRT wrapper for the D3D device.

    // Windows.Graphics.Capture resources
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem captureItem_{nullptr}; // The item to be captured (a specific window).
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool framePool_{nullptr}; // The pool that manages captured frames.
    winrt::Windows::Graphics::Capture::GraphicsCaptureSession session_{nullptr}; // The capture session itself.
    winrt::event_token frameArrivedToken_{}; // Token for the FrameArrived event registration.

    // Frame data and synchronization
    cv::Mat frame_; // The cv::Mat that stores the latest captured frame data (in BGRA format).
    std::mutex frameMutex_; // Mutex to protect access to frame_ and frameReady_.
    bool frameReady_ = false; // Flag indicating if a new frame is ready to be retrieved.
    bool isCapturing_ = false; // Flag indicating if the capture session is currently active.
};
} // namespace sba