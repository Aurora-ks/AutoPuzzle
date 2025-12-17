#include "Capture.h"
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <iostream>
#include <stdexcept>

namespace sba {

Capture::Capture(HWND hwnd) {
    if (!IsWindow(hwnd)) throw std::runtime_error("Invalid window handle.");
    window_ = hwnd;
    // Set the process to be DPI-aware. This is crucial for correct scaling.
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
}

Capture::~Capture() {
    stop();
}

// start: Launches the capture thread and waits for it to initialize.
void Capture::start() {
    if (bIsThreadRunning_) return;

    bIsThreadRunning_ = true;
    captureThread_ = std::thread(&Capture::captureThread_, this);

    // Wait for the capture thread to finish initialization.
    std::unique_lock lock(initMutex_);
    initCv_.wait(lock, [this] { return bIsInitialized_; });
}

// stop: Signals the capture thread to terminate and waits for it to exit.
void Capture::stop() {
    if (!bIsThreadRunning_) return;

    bIsThreadRunning_ = false;

    // Post a WM_QUIT message to the thread's message queue to unblock GetMessage.
    if (captureThread_.joinable()) {
        PostThreadMessage(GetThreadId(captureThread_.native_handle()), WM_QUIT, 0, 0);
        captureThread_.join();
    }
    bIsInitialized_ = false;
}

cv::Mat Capture::waitForNextFrame(int timeoutMs) {
    std::unique_lock lock(frameMutex_);
    if (!frameCv_.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this] { return bFrameReady_; })) {
        return cv::Mat();  // 超时返回空Mat
    }
    cv::Mat newFrame = frame_.clone();
    bFrameReady_ = false;
    lock.unlock();
    cv::cvtColor(newFrame, newFrame, cv::COLOR_BGRA2BGR);
    return newFrame;
}

// This is the entry point for the background capture thread.
void Capture::captureThread() {
    try {
        // Initialize the COM apartment for this thread.
        winrt::init_apartment(winrt::apartment_type::multi_threaded);

        // --- All resource creation is now done on this thread ---
        d3dDevice_ = createD3DDevice();
        d3dDevice_->GetImmediateContext(d3dContext_.put());

        // Get the DXGI device and create a WinRT IDirect3DDevice for interop.
        winrt::com_ptr<IDXGIDevice> dxgiDevice = d3dDevice_.as<IDXGIDevice>();
        winrt::com_ptr<::IInspectable> d3d_device_inspectable;
        winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.get(), d3d_device_inspectable.put()));
        direct3DDevice_ = d3d_device_inspectable.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();

        // Create a GraphicsCaptureItem for the target window.
        captureItem_ = createCaptureItemForWindow(window_);

        // Create a frame pool to store captured frames.
        // The frames are stored in B8G8R8A8 format, which is compatible with OpenCV's BGRA format.
        framePool_ = winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::Create(direct3DDevice_,
            winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
            2,  // Number of buffers in the frame pool.
            captureItem_.Size());

        session_ = framePool_.CreateCaptureSession(captureItem_);
        session_.IsCursorCaptureEnabled(false);
        frameArrivedToken_ = framePool_.FrameArrived({this, &Capture::onFrameArrived});

        // --- Initialization is complete ---
        {
            std::lock_guard lock(initMutex_);
            bIsInitialized_ = true;
        }
        initCv_.notify_one();  // Signal the main thread that we are ready.
        std::cout << "Capture thread initialized successfully." << std::endl;

        session_.StartCapture();
        std::cout << "Capture started." << std::endl;

        // --- Run the message loop ---
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

    } catch (const winrt::hresult_error& ex) {
        std::wcerr << L"Capture thread failed: " << ex.message().c_str() << std::endl;
    } catch (const std::exception& ex) {
        std::cerr << "Capture thread failed: " << ex.what() << std::endl;
    }

    // --- Cleanup ---
    cleanup();
    winrt::uninit_apartment();
    std::cout << "Capture thread finished." << std::endl;
}

void Capture::onFrameArrived(
    const winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool& sender, const winrt::Windows::Foundation::IInspectable& args) {
    if (!bIsThreadRunning_) return;

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

    cv::Mat frame(desc.Height, desc.Width, CV_8UC4, mapped.pData, mapped.RowPitch);  // share with mapped.pData

    RECT windowRect;
    GetWindowRect(window_, &windowRect);  // Get full window dimensions in screen coordinates

    RECT clientRect;
    GetClientRect(window_, &clientRect);  // Get client area dimensions in client coordinates
    // Convert clientRect to screen coordinates for direct comparison with windowRect
    MapWindowPoints(window_, HWND_DESKTOP, (LPPOINT) &clientRect, 2);

    // Now, calculate the offsets for cropping
    // int cropOffsetX = clientRect.left - windowRect.left;
    int cropOffsetX = 0;
    int cropOffsetY = clientRect.top - windowRect.top;
    int cropWidth = clientRect.right - clientRect.left;
    int cropHeight = clientRect.bottom - clientRect.top;

    cv::Rect roi(cropOffsetX, cropOffsetY, cropWidth, cropHeight);
    if (roi.x >= 0 && roi.y >= 0 && roi.x + roi.width <= frame.cols && roi.y + roi.height <= frame.rows) {
        {
            std::lock_guard lock(frameMutex_);
            frame_ = frame(roi).clone();
            bFrameReady_ = true;
        }
        frameCv_.notify_one();
    } else {
        throw std::runtime_error(
            std::format("Captrure ROI[{},{},{},{}] is out of bounds for frame[{}, {}]", roi.x, roi.y, roi.width, roi.height, frame.cols, frame.rows));
    }
    d3dContext_->Unmap(stagingTexture.get(), 0);
}

void Capture::cleanup() {
    if (session_) {
        session_.Close();
        session_ = nullptr;
    }
    if (framePool_) {
        if (frameArrivedToken_.value != 0) {
            framePool_.FrameArrived(frameArrivedToken_);
            frameArrivedToken_ = {};
        }
        framePool_.Close();
        framePool_ = nullptr;
    }
    captureItem_ = nullptr;
    d3dContext_ = nullptr;
    d3dDevice_ = nullptr;
    direct3DDevice_ = nullptr;
    std::cout << "Capture resources released." << std::endl;
}

winrt::com_ptr<ID3D11Device> Capture::createD3DDevice() {
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
    winrt::check_hresult(D3D11CreateDevice(nullptr,  // Use default adapter.
        D3D_DRIVER_TYPE_HARDWARE,  // Use hardware acceleration.
        nullptr,  // No software rasterizer.
        creationFlags,  // Device creation flags.
        featureLevels,  // Array of feature levels.
        ARRAYSIZE(featureLevels),  // Size of the feature levels array.
        D3D11_SDK_VERSION,  // SDK version.
        device.put(),  // Pointer to receive the device.
        nullptr,  // Pointer to receive the feature level.
        nullptr  // Pointer to receive the device context.
        ));

    return device;
}

winrt::Windows::Graphics::Capture::GraphicsCaptureItem Capture::createCaptureItemForWindow(HWND hwnd) {
    // Get the activation factory for GraphicsCaptureItem.
    auto activation_factory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem>();
    // Get the interop interface to create a capture item from an HWND.
    auto interop = activation_factory.as<IGraphicsCaptureItemInterop>();
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem item = {nullptr};
    // Create the capture item for the specified window.
    winrt::check_hresult(
        interop->CreateForWindow(hwnd, winrt::guid_of<winrt::Windows::Graphics::Capture::GraphicsCaptureItem>(), winrt::put_abi(item)));
    return item;
}

}  // namespace sba