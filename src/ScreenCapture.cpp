#include "ScreenCapture.h"
#include <iostream>
#include <stdexcept>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>

namespace psa {

ScreenCapture::ScreenCapture() {
    HWND hwnd = FindWindowW(L"UnrealWindow", L"尘白禁区");
    if (!hwnd) {
        std::cerr << "Could not find the window." << std::endl;
        throw std::runtime_error("Could not find the window.");
    }
    try {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);

        d3dDevice_ = CreateD3DDevice();
        d3dDevice_->GetImmediateContext(d3dContext_.put());

        winrt::com_ptr<IDXGIDevice> dxgiDevice = d3dDevice_.as<IDXGIDevice>();
        winrt::com_ptr<::IInspectable> d3d_device_inspectable;
        winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.get(), d3d_device_inspectable.put()));
        direct3DDevice_ = d3d_device_inspectable.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();

        captureItem_ = CreateCaptureItemForWindow(hwnd);
        
        framePool_ = winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::Create(
            direct3DDevice_,
            winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
            2,
            captureItem_.Size());
            
        session_ = framePool_.CreateCaptureSession(captureItem_);
        session_.IsCursorCaptureEnabled(false);
        
        frameArrivedToken_ = framePool_.FrameArrived({this, &ScreenCapture::OnFrameArrived});

    } catch (const winrt::hresult_error& ex) {
        std::wcerr << L"ScreenCapture initialization failed: " << ex.message().c_str() << std::endl;
        Cleanup();
        throw std::runtime_error("ScreenCapture initialization failed.");
    }
}

ScreenCapture::~ScreenCapture() {
    Cleanup();
}

void ScreenCapture::start() {
    if (session_ && !isCapturing_) {
        session_.StartCapture();
        isCapturing_ = true;
        std::cout << "Capture started." << std::endl;
    }
}

void ScreenCapture::stop() {
    if (isCapturing_) {
        if (session_) {
            session_.Close();
            session_ = nullptr;
        }
        isCapturing_ = false;
        std::cout << "Capture stopped." << std::endl;
    }
}

void ScreenCapture::Cleanup() {
    stop();
    if (framePool_ && frameArrivedToken_.value != 0) {
        framePool_.FrameArrived(frameArrivedToken_);
        frameArrivedToken_ = {};
    }
    
    session_ = nullptr;
    
    if (framePool_) {
        framePool_.Close();
        framePool_ = nullptr;
    }
    captureItem_ = nullptr;
    d3dContext_ = nullptr;
    d3dDevice_ = nullptr;
    direct3DDevice_ = nullptr;
    std::cout << "ScreenCapture resources released." << std::endl;
}

cv::Mat ScreenCapture::GetLatestFrame() {
    cv::Mat frame;
    std::lock_guard lock(frameMutex_);
    if (!frameReady_) return frame;

    cv::cvtColor(frame_, frame, cv::COLOR_BGRA2BGR);
    frameReady_ = false;
    return frame;
}

void ScreenCapture::OnFrameArrived(
    const winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool& sender,
    const winrt::Windows::Foundation::IInspectable& args) {

    auto current_frame = sender.TryGetNextFrame();
    if (!current_frame) return;

    winrt::com_ptr<ID3D11Texture2D> sourceTexture;
    auto access = current_frame.Surface().as<Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
    winrt::check_hresult(access->GetInterface(winrt::guid_of<ID3D11Texture2D>(), sourceTexture.put_void()));

    D3D11_TEXTURE2D_DESC desc;
    sourceTexture->GetDesc(&desc);

    D3D11_TEXTURE2D_DESC stagingDesc = desc;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.MiscFlags = 0;

    winrt::com_ptr<ID3D11Texture2D> stagingTexture;
    winrt::check_hresult(d3dDevice_->CreateTexture2D(&stagingDesc, nullptr, stagingTexture.put()));

    d3dContext_->CopyResource(stagingTexture.get(), sourceTexture.get());

    D3D11_MAPPED_SUBRESOURCE mapped;
    winrt::check_hresult(d3dContext_->Map(stagingTexture.get(), 0, D3D11_MAP_READ, 0, &mapped));

    {
        std::lock_guard lock(frameMutex_);
        if (frame_.empty() || frame_.cols != desc.Width || frame_.rows != desc.Height) {
            frame_ = cv::Mat(desc.Height, desc.Width, CV_8UC4);
        }
        
        uint8_t* pDst = frame_.data;
        const uint8_t* pSrc = static_cast<const uint8_t*>(mapped.pData);
        const UINT srcRowPitch = mapped.RowPitch;
        const UINT dstRowPitch = frame_.step;

        for (UINT y = 0; y < desc.Height; ++y) {
            memcpy(pDst, pSrc, dstRowPitch);
            pDst += dstRowPitch;
            pSrc += srcRowPitch;
        }
        frameReady_ = true;
    }

    d3dContext_->Unmap(stagingTexture.get(), 0);
}

winrt::com_ptr<ID3D11Device> ScreenCapture::CreateD3DDevice() {
    winrt::com_ptr<ID3D11Device> device;
    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    winrt::check_hresult(D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, creationFlags, featureLevels,
        ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, device.put(), nullptr, nullptr));

    return device;
}

winrt::Windows::Graphics::Capture::GraphicsCaptureItem ScreenCapture::CreateCaptureItemForWindow(HWND hwnd) {
    auto activation_factory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem>();
    auto interop = activation_factory.as<IGraphicsCaptureItemInterop>();
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem item = {nullptr};
    winrt::check_hresult(interop->CreateForWindow(hwnd, winrt::guid_of<winrt::Windows::Graphics::Capture::GraphicsCaptureItem>(), winrt::put_abi(item)));
    return item;
}

} // namespace psa