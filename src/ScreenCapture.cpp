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

        // Create the Direct3D 11 device and get its immediate context.
        d3dDevice_ = CreateD3DDevice();
        d3dDevice_->GetImmediateContext(d3dContext_.put());

        // Get the DXGI device and create a WinRT IDirect3DDevice for interop.
        winrt::com_ptr<IDXGIDevice> dxgiDevice = d3dDevice_.as<IDXGIDevice>();
        winrt::com_ptr<::IInspectable> d3d_device_inspectable;
        winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.get(), d3d_device_inspectable.put()));
        direct3DDevice_ = d3d_device_inspectable.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();

        // Create a GraphicsCaptureItem for the target window.
        captureItem_ = CreateCaptureItemForWindow(hwnd);
        
        // Create a frame pool to store captured frames.
        // The frames are stored in B8G8R8A8 format, which is compatible with OpenCV's BGRA format.
        framePool_ = winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::Create(
            direct3DDevice_,
            winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
            2, // Number of buffers in the frame pool.
            captureItem_.Size()); // Size of the capture item.

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

    // Get the latest frame from the pool.
    auto current_frame = sender.TryGetNextFrame();
    if (!current_frame) return;

    // Get the D3D11 texture from the frame.
    winrt::com_ptr<ID3D11Texture2D> sourceTexture;
    auto access = current_frame.Surface().as<Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
    winrt::check_hresult(access->GetInterface(winrt::guid_of<ID3D11Texture2D>(), sourceTexture.put_void()));

    D3D11_TEXTURE2D_DESC desc;
    sourceTexture->GetDesc(&desc);

    // Create a staging texture that is accessible by the CPU.
    D3D11_TEXTURE2D_DESC stagingDesc = desc;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.MiscFlags = 0;

    winrt::com_ptr<ID3D11Texture2D> stagingTexture;
    winrt::check_hresult(d3dDevice_->CreateTexture2D(&stagingDesc, nullptr, stagingTexture.put()));

    // Copy the GPU texture to the CPU-accessible staging texture.
    d3dContext_->CopyResource(stagingTexture.get(), sourceTexture.get());

    // Map the staging texture to access its data.
    D3D11_MAPPED_SUBRESOURCE mapped;
    winrt::check_hresult(d3dContext_->Map(stagingTexture.get(), 0, D3D11_MAP_READ, 0, &mapped));

    {
        // Lock the mutex to safely write to the shared cv::Mat.
        std::lock_guard lock(frameMutex_);
        // Re-allocate the Mat if the size has changed.
        if (frame_.empty() || frame_.cols != desc.Width || frame_.rows != desc.Height) {
            frame_ = cv::Mat(desc.Height, desc.Width, CV_8UC4);
        }
        
        // Get pointers to the source and destination data.
        uint8_t* pDst = frame_.data;
        const uint8_t* pSrc = static_cast<const uint8_t*>(mapped.pData);
        const UINT srcRowPitch = mapped.RowPitch; // Stride of the source texture.
        const UINT dstRowPitch = frame_.step; // Stride of the destination cv::Mat.

        // Copy the pixel data row by row.
        // This is necessary if the row pitch (stride) of the texture and the cv::Mat are different.
        for (UINT y = 0; y < desc.Height; ++y) {
            memcpy(pDst, pSrc, dstRowPitch);
            pDst += dstRowPitch;
            pSrc += srcRowPitch;
        }
        frameReady_ = true;
    }

    // Unmap the texture.
    d3dContext_->Unmap(stagingTexture.get(), 0);
}

winrt::com_ptr<ID3D11Device> ScreenCapture::CreateD3DDevice() {
    winrt::com_ptr<ID3D11Device> device;
    // Enable BGRA support for compatibility with Windows.Graphics.Capture.
    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
    // Enable debug layer in debug builds.
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    // Define the feature levels that the application supports.
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    // Create the D3D11 device.
    winrt::check_hresult(D3D11CreateDevice(
        nullptr, // Use default adapter.
        D3D_DRIVER_TYPE_HARDWARE, // Use hardware acceleration.
        nullptr, // No software rasterizer.
        creationFlags, // Device creation flags.
        featureLevels, // Array of feature levels.
        ARRAYSIZE(featureLevels), // Size of the feature levels array.
        D3D11_SDK_VERSION, // SDK version.
        device.put(), // Pointer to receive the device.
        nullptr, // Pointer to receive the feature level.
        nullptr // Pointer to receive the device context.
    ));

    return device;
}

winrt::Windows::Graphics::Capture::GraphicsCaptureItem ScreenCapture::CreateCaptureItemForWindow(HWND hwnd) {
    // Get the activation factory for GraphicsCaptureItem.
    auto activation_factory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem>();
    // Get the interop interface to create a capture item from an HWND.
    auto interop = activation_factory.as<IGraphicsCaptureItemInterop>();
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem item = {nullptr};
    // Create the capture item for the specified window.
    winrt::check_hresult(interop->CreateForWindow(hwnd, winrt::guid_of<winrt::Windows::Graphics::Capture::GraphicsCaptureItem>(), winrt::put_abi(item)));
    return item;
}

} // namespace psa
