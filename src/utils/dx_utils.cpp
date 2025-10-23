#include "pch.h"
#include "dx_utils.h"

namespace utils
{
    const wchar_t* DXUtils::TEMP_WINDOW_CLASS = L"DXUtilsTempWindow";
    bool DXUtils::s_windowClassRegistered = false;

    RenderAPI DXUtils::getRenderAPI()
    {
        HMODULE d3d11 = getD3D11Module();
        HMODULE d3d12 = getD3D12Module();

        if (d3d11) return RenderAPI::DirectX11;
        if (d3d12) return RenderAPI::DirectX12;

        return RenderAPI::Unknown;
    }

    HMODULE DXUtils::getD3D11Module()
    {
        return GetModuleHandleW(L"d3d11.dll");
    }

    HMODULE DXUtils::getD3D12Module()
    {
        return GetModuleHandleW(L"d3d12.dll");
    }

    HMODULE DXUtils::getDXGIModule()
    {
        return GetModuleHandleW(L"dxgi.dll");
    }

    bool DXUtils::createTempD3D11Device(HWND hwnd, ID3D11Device** device, ID3D11DeviceContext** context,
                                        IDXGISwapChain** swapChain)
    {
        HMODULE d3d11Module = getD3D11Module();
        if (!d3d11Module) return false;

        using D3D11CreateDeviceAndSwapChain_t = HRESULT(WINAPI*)(
            IDXGIAdapter* pAdapter,
            D3D_DRIVER_TYPE DriverType,
            HMODULE Software,
            UINT Flags,
            const D3D_FEATURE_LEVEL* pFeatureLevels,
            UINT FeatureLevels,
            UINT SDKVersion,
            const DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
            IDXGISwapChain** ppSwapChain,
            ID3D11Device** ppDevice,
            D3D_FEATURE_LEVEL* pFeatureLevel,
            ID3D11DeviceContext** ppImmediateContext);

        auto D3D11CreateDeviceAndSwapChain = (D3D11CreateDeviceAndSwapChain_t)GetProcAddress(
            d3d11Module, "D3D11CreateDeviceAndSwapChain");
        if (!D3D11CreateDeviceAndSwapChain) return false;

        DXGI_RATIONAL refreshRate;
        refreshRate.Numerator = 60;
        refreshRate.Denominator = 1;

        DXGI_MODE_DESC bufferDesc;
        bufferDesc.Width = 800;
        bufferDesc.Height = 600;
        bufferDesc.RefreshRate = refreshRate;
        bufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        bufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        bufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        swapChainDesc.BufferDesc = bufferDesc;
        swapChainDesc.BufferCount = 1;
        swapChainDesc.BufferDesc.Width = 1;
        swapChainDesc.BufferDesc.Height = 1;
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.OutputWindow = hwnd;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Windowed = TRUE;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        D3D_FEATURE_LEVEL featureLevel;
        constexpr D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1};

        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr, // Use default adapter
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr, // No software rasterizer
            0, // No flags
            featureLevels, // Feature levels to support
            ARRAYSIZE(featureLevels), // Number of feature levels
            D3D11_SDK_VERSION,
            &swapChainDesc,
            swapChain,
            device,
            &featureLevel,
            context);

        return SUCCEEDED(hr);
    }

    // Reference: https://github.com/Sh0ckFR/Universal-Dear-ImGui-Hook/blob/e65aacad5a88eb0a2f7d52e94c5cced93f2f0436/hooks.cpp
    bool DXUtils::createTempD3D12Device(HWND hwnd, ID3D12Device** device, ID3D12CommandQueue** commandQueue,
                                        IDXGISwapChain3** swapChain)
    {
        HMODULE d3d12Module = getD3D12Module();
        HMODULE dxgiModule = getDXGIModule();
        if (!d3d12Module || !dxgiModule) return false;

        using D3D12CreateDevice_t = HRESULT(WINAPI*)(
            IUnknown* pAdapter,
            D3D_FEATURE_LEVEL MinimumFeatureLevel,
            REFIID riid,
            void** ppDevice);
        using CreateDXGIFactory1_t = HRESULT(WINAPI*)(
            REFIID riid,
            void** ppFactory);

        auto D3D12CreateDevice = (D3D12CreateDevice_t)GetProcAddress(d3d12Module, "D3D12CreateDevice");
        auto CreateDXGIFactory1 = (CreateDXGIFactory1_t)GetProcAddress(dxgiModule, "CreateDXGIFactory1");
        if (!D3D12CreateDevice || !CreateDXGIFactory1) return false;

        IDXGIFactory4* factory = nullptr;
        HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
        if (FAILED(hr)) return false;

        hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(device));
        if (FAILED(hr))
        {
            factory->Release();
            return false;
        }

        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queueDesc.Priority = 0;
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.NodeMask = 0;

        hr = (*device)->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(commandQueue));
        if (FAILED(hr))
        {
            (*device)->Release();
            *device = nullptr;
            factory->Release();
            return false;
        }

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = 1;
        swapChainDesc.Height = 1;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = 2;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.Flags = 0;

        IDXGISwapChain1* swapChain1 = nullptr;
        hr = factory->CreateSwapChainForHwnd(
            *commandQueue,
            hwnd,
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain1);
        if (FAILED(hr) || !swapChain1)
        {
            (*commandQueue)->Release();
            *commandQueue = nullptr;
            (*device)->Release();
            *device = nullptr;
            factory->Release();
            return false;
        }

        hr = swapChain1->QueryInterface(IID_PPV_ARGS(swapChain));

        // Always release swapChain1 after querying
        swapChain1->Release();
        factory->Release();

        if (FAILED(hr) || !swapChain)
        {
            (*commandQueue)->Release();
            *commandQueue = nullptr;
            (*device)->Release();
            *device = nullptr;
            return false;
        }

        return SUCCEEDED(hr);
    }

    HWND DXUtils::createTempWindow()
    {
        if (!s_windowClassRegistered) registerTempWindowClass();

        return CreateWindowW(
            TEMP_WINDOW_CLASS,
            L"Temp Window",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, 1, 1,
            nullptr,
            nullptr,
            GetModuleHandleW(nullptr),
            nullptr);
    }

    void DXUtils::destroyTempWindow(HWND hwnd)
    {
        if (hwnd) DestroyWindow(hwnd);
    }

    void DXUtils::registerTempWindowClass()
    {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = tempWindowProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = TEMP_WINDOW_CLASS;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);

        RegisterClassW(&wc);
        s_windowClassRegistered = true;
    }

    LRESULT CALLBACK DXUtils::tempWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}
