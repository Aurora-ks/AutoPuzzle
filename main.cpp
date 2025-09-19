#include <iostream>
#include <mutex>
#include <windows.h>
#include <dwmapi.h>
#include <Unknwn.h> // For IUnknown

// OpenCV
#include <opencv2/opencv.hpp>

// WinRT
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.Graphics.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>

// DirectX
#include <d3d11.h>
#include <dxgi1_2.h>

// WinRT-DirectX Interop
#include <windows.graphics.capture.interop.h> // For IGraphicsCaptureItemInterop
#include <windows.graphics.directx.direct3d11.interop.h> // For IDirect3DDxgiInterfaceAccess

// #pragma comment(lib, "dwmapi.lib")
// #pragma comment(lib, "d3d11.lib")
// #pragma comment(lib, "dxgi.lib")
// #pragma comment(lib, "windowsapp.lib")

// Function prototypes
winrt::com_ptr<ID3D11Device> CreateD3DDevice();
winrt::Windows::Graphics::Capture::GraphicsCaptureItem CreateCaptureItemForWindow(HWND hwnd);

int main()
{
    // Set the process to be DPI-aware. This is crucial for correct scaling.
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    try
    {
        // 1. Initialize WinRT and COM
        winrt::init_apartment(winrt::apartment_type::multi_threaded);

        // 2. Find the window to capture.
        HWND hwnd = FindWindowW(L"UnrealWindow", L"尘白禁区");
        if (!hwnd)
        {
            std::cerr << "Could not find a window" << std::endl;
            return 1;
        }
        
        std::cout << "Found window, starting capture..." << std::endl;

        // 3. Set up DirectX device
        auto d3dDevice = CreateD3DDevice();
        winrt::com_ptr<ID3D11DeviceContext> d3dContext;
        d3dDevice->GetImmediateContext(d3dContext.put());

        winrt::com_ptr<IDXGIDevice> dxgiDevice = d3dDevice.as<IDXGIDevice>();
        winrt::com_ptr<::IInspectable> d3d_device_inspectable;
        winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.get(), d3d_device_inspectable.put()));
        auto direct3DDevice = d3d_device_inspectable.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();

        // 4. Create GraphicsCaptureItem
        auto captureItem = CreateCaptureItemForWindow(hwnd);
        auto size = captureItem.Size();

        // 5. Create frame pool and capture session
        auto framePool = winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::Create(
            direct3DDevice,
            winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
            2, // Number of buffers
            size);
        auto session = framePool.CreateCaptureSession(captureItem);

        cv::Mat frame;
        std::mutex frameMutex;
        bool frameReady = false;

        // 6. Set up the frame arrival event handler
        framePool.FrameArrived([&](auto&, auto&)
        {
            auto current_frame = framePool.TryGetNextFrame();
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
            winrt::check_hresult(d3dDevice->CreateTexture2D(&stagingDesc, nullptr, stagingTexture.put()));

            d3dContext->CopyResource(stagingTexture.get(), sourceTexture.get());

            D3D11_MAPPED_SUBRESOURCE mapped;
            winrt::check_hresult(d3dContext->Map(stagingTexture.get(), 0, D3D11_MAP_READ, 0, &mapped));

            {
                std::lock_guard<std::mutex> lock(frameMutex);
                if (frame.empty() || frame.cols != desc.Width || frame.rows != desc.Height)
                {
                    frame = cv::Mat(desc.Height, desc.Width, CV_8UC4);
                }
                // Copy the data row by row, respecting the RowPitch from the source texture.
                uint8_t* pDst = frame.data;
                const uint8_t* pSrc = static_cast<const uint8_t*>(mapped.pData);
                const UINT srcRowPitch = mapped.RowPitch;
                const UINT dstRowPitch = frame.step;

                for (UINT y = 0; y < desc.Height; ++y)
                {
                    memcpy(pDst, pSrc, dstRowPitch);
                    pDst += dstRowPitch;
                    pSrc += srcRowPitch;
                }
                frameReady = true;
            }

            d3dContext->Unmap(stagingTexture.get(), 0);
        });

        // 7. Start capturing
        session.StartCapture();
        std::cout << "Capture started. Press 'q' in the display window to quit." << std::endl;

        // 8. Main loop to display frames
        while (true)
        {
            bool hasFrame = false;
            cv::Mat displayFrame;

            {
                std::lock_guard<std::mutex> lock(frameMutex);
                if (frameReady)
                {
                    cv::cvtColor(frame, displayFrame, cv::COLOR_BGRA2BGR);
                    hasFrame = true;
                    frameReady = false;
                }
            }

            if (hasFrame)
            {
                // cv::imshow("Screen Capture", displayFrame);
                cv::imwrite("screen.png", displayFrame);
            }

            if (cv::waitKey(10) == 'q')
            {
                break;
            }
        }

        // 9. Clean up
        std::cout << "Stopping capture..." << std::endl;
        session.Close();
        framePool.Close();
        cv::destroyAllWindows();
    }
    catch (winrt::hresult_error const& ex)
    {
        std::wcerr << L"An exception occurred: " << ex.message().c_str() << std::endl;
        return 1;
    }
    return 0;
}

// Helper to create a D3D11 device
winrt::com_ptr<ID3D11Device> CreateD3DDevice()
{
    winrt::com_ptr<ID3D11Device> device;
    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    winrt::check_hresult(D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, creationFlags, featureLevels,
        ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, device.put(), nullptr, nullptr));

    return device;
}

// Helper to create a GraphicsCaptureItem from a window handle (HWND)
winrt::Windows::Graphics::Capture::GraphicsCaptureItem CreateCaptureItemForWindow(HWND hwnd)
{
    auto activation_factory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem>();
    auto interop = activation_factory.as<IGraphicsCaptureItemInterop>();
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem item = { nullptr };
    winrt::check_hresult(interop->CreateForWindow(hwnd, winrt::guid_of<winrt::Windows::Graphics::Capture::GraphicsCaptureItem>(), reinterpret_cast<void**>(winrt::put_abi(item))));
    return item;
}