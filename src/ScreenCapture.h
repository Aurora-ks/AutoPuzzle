#pragma once

#include <windows.h>
#include <mutex>
#include <opencv2/opencv.hpp>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <d3d11.h>
#include <dxgi1_2.h>

namespace psa {
class ScreenCapture {
public:
    explicit ScreenCapture();
    ~ScreenCapture();

    void start();
    void stop();
    cv::Mat GetLatestFrame();

private:
    void OnFrameArrived(
        const winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool& sender,
        const winrt::Windows::Foundation::IInspectable& args);
    
    void Cleanup();

    static winrt::com_ptr<ID3D11Device> CreateD3DDevice();
    static winrt::Windows::Graphics::Capture::GraphicsCaptureItem CreateCaptureItemForWindow(HWND hwnd);

    winrt::com_ptr<ID3D11Device> d3dDevice_;
    winrt::com_ptr<ID3D11DeviceContext> d3dContext_;
    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice direct3DDevice_{nullptr};
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem captureItem_{nullptr};
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool framePool_{nullptr};
    winrt::Windows::Graphics::Capture::GraphicsCaptureSession session_{nullptr};
    winrt::event_token frameArrivedToken_{};

    cv::Mat frame_;
    std::mutex frameMutex_;
    bool frameReady_ = false;
    bool isCapturing_ = false;
};
} // namespace psa