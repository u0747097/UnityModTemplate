#pragma once
#include <d3d11.h>
#include <d3d12.h>
#include <d3d9.h>
#include <dxgi.h>
#include <dxgi1_4.h>

enum class RenderAPI
{
    Unknown = 0,
    DirectX11 = 2,
    DirectX12 = 3,
};

namespace utils
{
    class DXUtils
    {
    public:
        static RenderAPI getRenderAPI();

        static HMODULE getD3D11Module();
        static HMODULE getD3D12Module();
        static HMODULE getDXGIModule();

        static bool createTempD3D11Device(HWND hwnd, ID3D11Device** device, ID3D11DeviceContext** context,
                                          IDXGISwapChain** swapChain);
        static bool createTempD3D12Device(HWND hwnd, ID3D12Device** device, ID3D12CommandQueue** commandQueue,
                                          IDXGISwapChain3** swapChain);

        static HWND createTempWindow();
        static void destroyTempWindow(HWND hwnd);

    private:
        static const wchar_t* TEMP_WINDOW_CLASS;
        static bool s_windowClassRegistered;

        static void registerTempWindowClass();
        static LRESULT CALLBACK tempWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    };
}
