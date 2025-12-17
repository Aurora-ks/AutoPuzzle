#pragma once

#include <d3d11.h>
#include <dxgi1_2.h>
#include <windows.h>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <opencv2/opencv.hpp>
#include <thread>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>

namespace sba {
class Capture {
   public:
    explicit Capture(HWND hwnd);
    ~Capture();

    void start();
    void stop();
    cv::Mat waitForNextFrame(int timeoutS = 3);

   private:
    // Thread entry point
    void captureThread();

    // Event handler
    void onFrameArrived(
        const winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool& sender, const winrt::Windows::Foundation::IInspectable& args);

    // Internal state and resources
    void cleanup();
    static winrt::com_ptr<ID3D11Device> createD3DDevice();
    static winrt::Windows::Graphics::Capture::GraphicsCaptureItem createCaptureItemForWindow(HWND hwnd);

    // D3D resources (used by capture thread)
    winrt::com_ptr<ID3D11Device> d3dDevice_;
    winrt::com_ptr<ID3D11DeviceContext> d3dContext_;
    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice direct3DDevice_{nullptr};  // WinRT wrapper for the D3D device.

    // Capture resources (used by capture thread)
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem captureItem_{nullptr};
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool framePool_{nullptr};
    winrt::Windows::Graphics::Capture::GraphicsCaptureSession session_{nullptr};
    winrt::event_token frameArrivedToken_{};  // Token for the FrameArrived event registration.

    // Frame data synchronization (thread-safe)
    cv::Mat frame_;  // The cv::Mat that stores the latest captured frame data (in BGRA format).
    std::mutex frameMutex_;  // Mutex to protect access to frame_ and bFrameReady_.
    std::condition_variable frameCv_;  // Signals when a new frame is ready for WaitForNextFrame
    bool bFrameReady_ = false;

    // Thread management and initialization synchronization
    std::thread captureThread_;
    std::atomic<bool> bIsThreadRunning_ = false;
    std::mutex initMutex_;
    std::condition_variable initCv_;  // Signals when the capture thread has finished initialization
    bool bIsInitialized_ = false;

    // window handler
    HWND window_;
};

}  // namespace sba