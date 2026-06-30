// This code is based on MCBE-DLL-Menu by h-arvs
// Source: https://github.com/h-arvs/MCBE-DLL-Menu
// Licensed under CC BY 4.0: https://creativecommons.org/licenses/by/4.0/
// Changes were made.

#include <windows.h>
#include <wrl/client.h>

#include "MinHook.h"
#include "api/memory/HookAPI.hpp"
#include "../lib/kiero-src/include/kiero.hpp"
#include <winrt/base.h>
#include <safetyhook.hpp>
using Microsoft::WRL::ComPtr;

#include <atomic>
#include <cstdio>
#include <d3d11.h>
#include <d3d12.h>
#include <dxgi1_5.h>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "backends/imgui_impl_dx12.h"
#include "backends/imgui_impl_win32.h"
#include <imgui_impl_dx11.h>

#include "ImGuiHooks.h"
#include "api/memory/Hook.h"
#include "gui/GUI.h"
#include <d3d11on12.h>

HWND window{};
bool uninject = false;

winrt::com_ptr<ID3D11Device> g_d3d11Device{};
winrt::com_ptr<ID3D11On12Device> g_d3d11on12Device{};
winrt::com_ptr<ID3D11DeviceContext> g_d3d11DeviceContext{};
winrt::com_ptr<ID3D12CommandQueue> g_d3d12CommandQueue{};

struct BufferData {
    winrt::com_ptr<ID3D12Resource> native;
    winrt::com_ptr<ID3D11Resource> wrapped;
};

static std::vector<BufferData> wrappedBuffers;

safetyhook::InlineHook presentHookImpl{};

void resetD3DState() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    wrappedBuffers.clear();
    g_d3d11DeviceContext = nullptr;
    g_d3d11on12Device = nullptr;
    g_d3d11Device = nullptr;
}

HRESULT presentHook(IDXGISwapChain3 *pSwapChain, UINT SyncInterval, UINT Flags) {
    if (!g_d3d11Device) {
        winrt::com_ptr<ID3D12Device> d3d12Device{};
        if (SUCCEEDED(pSwapChain->GetDevice(IID_PPV_ARGS(d3d12Device.put())))) {
            UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_SINGLETHREADED;

            auto commandQueue = g_d3d12CommandQueue.get();

            if (!commandQueue) {
                return presentHookImpl.call<HRESULT>(pSwapChain, SyncInterval, Flags);
            }

            HRESULT hr = D3D11On12CreateDevice(d3d12Device.get(), deviceFlags, nullptr, 0,
                                               reinterpret_cast<IUnknown *const *>(&commandQueue), 1, 0,
                                               g_d3d11Device.put(), g_d3d11DeviceContext.put(), nullptr);
            if (FAILED(hr)) {
                Logger::log("D3D11On12CreateDevice failed: 0x%08X", static_cast<unsigned int>(hr));
                return presentHookImpl.call<HRESULT>(pSwapChain, SyncInterval, Flags);
            }

            g_d3d11on12Device = g_d3d11Device.as<ID3D11On12Device>();

            Logger::log("Initialized d3d11on12");
        } else if (SUCCEEDED(pSwapChain->GetDevice(IID_PPV_ARGS(g_d3d11Device.put())))) {
            g_d3d11Device->GetImmediateContext(g_d3d11DeviceContext.put());
        } else {
            return presentHookImpl.call<HRESULT>(pSwapChain, SyncInterval, Flags);
        }

        initializeImGui(false);
        if (!ImGui_ImplDX11_Init(g_d3d11Device.get(), g_d3d11DeviceContext.get()) || !ImGui_ImplWin32_Init(window)) {
            Logger::log("Failed to initialize ImGui backends");
            return presentHookImpl.call<HRESULT>(pSwapChain, SyncInterval, Flags);
        }
        Logger::log("Initialized ImGui");
    }

    winrt::com_ptr<ID3D11Resource> backBuffer{};
    bool wrappedResourceAcquired = false;

    try {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();

        updateImGui();

        ImGui::Render();

        if (g_d3d11on12Device) {
            DXGI_SWAP_CHAIN_DESC desc;
            winrt::check_hresult(pSwapChain->GetDesc(&desc));
            UINT buffer_count = desc.BufferCount;
            if (buffer_count != wrappedBuffers.size()) {
                wrappedBuffers.clear();
                wrappedBuffers.resize(buffer_count);
                for (UINT i = 0; i < buffer_count; i++) {
                    auto &bufferData = wrappedBuffers.at(i);

                    winrt::check_hresult(pSwapChain->GetBuffer(i, IID_PPV_ARGS(bufferData.native.put())));

                    D3D11_RESOURCE_FLAGS resourceFlags{};
                    resourceFlags.BindFlags = D3D11_BIND_RENDER_TARGET;
                    winrt::check_hresult(g_d3d11on12Device->CreateWrappedResource(
                            bufferData.native.get(), &resourceFlags, D3D12_RESOURCE_STATE_RENDER_TARGET,
                            D3D12_RESOURCE_STATE_PRESENT, IID_PPV_ARGS(bufferData.wrapped.put())));
                }
                Logger::log("Wrapped resources");
            }

            backBuffer.copy_from(wrappedBuffers[pSwapChain->GetCurrentBackBufferIndex()].wrapped.get());
            auto backBufferPtr = backBuffer.get();
            g_d3d11on12Device->AcquireWrappedResources(&backBufferPtr, 1);
            wrappedResourceAcquired = true;
        } else {
            pSwapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.put()));
        }

        winrt::com_ptr<ID3D11RenderTargetView> g_mainRenderTargetView{};
        winrt::check_hresult(
                g_d3d11Device->CreateRenderTargetView(backBuffer.get(), nullptr, g_mainRenderTargetView.put()));

        auto renderTargetView = g_mainRenderTargetView.get();
        g_d3d11DeviceContext->OMSetRenderTargets(1, &renderTargetView, nullptr);

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        if (g_d3d11on12Device) {
            auto backBufferPtr = backBuffer.get();
            g_d3d11on12Device->ReleaseWrappedResources(&backBufferPtr, 1);
            wrappedResourceAcquired = false;
        }

        g_d3d11DeviceContext->Flush();
    } catch (const winrt::hresult_error &e) {
        Logger::log("D3D error in presentHook: 0x%08X", static_cast<unsigned int>(e.code()));
        if (wrappedResourceAcquired && g_d3d11on12Device && backBuffer) {
            auto backBufferPtr = backBuffer.get();
            g_d3d11on12Device->ReleaseWrappedResources(&backBufferPtr, 1);
            g_d3d11DeviceContext->Flush();
        }
        resetD3DState();
    }

    return presentHookImpl.call<HRESULT>(pSwapChain, SyncInterval, Flags);
};

safetyhook::InlineHook executeCommandListsHookImpl{};

void executeCommandListsHook(ID3D12CommandQueue *queue, UINT NumCommandLists,
                             ID3D12CommandList *const *ppCommandLists) {

    if (!g_d3d12CommandQueue) {
        g_d3d12CommandQueue.copy_from(queue);
        Logger::log("Got Command Queue");
    }

    return executeCommandListsHookImpl.call<void>(queue, NumCommandLists, ppCommandLists);
};

safetyhook::InlineHook resizeBuffers1HookImpl{};
HRESULT resizeBuffers1Hook(IDXGISwapChain *swapChain, UINT bufferCount, UINT width, UINT height, DXGI_FORMAT newFormat,
                           UINT swapChainFlags, const UINT *pCreationNodeMask, IUnknown *ppPresentQueue) {

    if (g_d3d11DeviceContext) {
        if (g_d3d11on12Device) {
            wrappedBuffers.resize(0);
        }

        g_d3d11DeviceContext->Flush();
    }

    return resizeBuffers1HookImpl.call<HRESULT>(swapChain, bufferCount, width, height, newFormat, swapChainFlags,
                                                pCreationNodeMask, ppPresentQueue);
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LONG_PTR wndProcO;
LRESULT wndProcHook(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        return true;

    return CallWindowProc(reinterpret_cast<WNDPROC>(wndProcO), hWnd, uMsg, wParam, lParam);
};

DeclareHook(Mouse,void, void *a1, void *a2, void *a3, void *a4) {
  if (ImGui::GetCurrentContext()) {
    ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
      return;
    }
  }
  return original(a1, a2, a3, a4);
}

DeclareHook(Mouse2,void, void* a1, char a2, char a3, __int16 a4, __int16 a5, __int16 a6, __int16 a7, char a8) {
  if (ImGui::GetCurrentContext()) {
    ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
      return;
    }
  }
  return original(a1, a2, a3, a4, a5, a6, a7, a8);
}

void initImGuiHooks() {
    window = FindWindowA("Bedrock", "Minecraft");

    if (!window) {
        Logger::log("Failed to get window");
    }

    wndProcO = SetWindowLongPtrA(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&wndProcHook));
    Logger::log("Hooked WndProc");

    if (kiero::init(kiero::RenderType::Auto) == kiero::Status::Success) {
        auto renderType = kiero::getRenderType();

        if (renderType == kiero::RenderType::D3D12) {
            Logger::log("D3D12 Detected");

            executeCommandListsHookImpl = safetyhook::create_inline(
                    kiero::getMethod<&ID3D12CommandQueue::ExecuteCommandLists>(), &executeCommandListsHook);

            Logger::log("Hooked executeCommandLists");

            resizeBuffers1HookImpl = safetyhook::create_inline(kiero::getMethod<&IDXGISwapChain3::ResizeBuffers1>(),
                                                               &resizeBuffers1Hook);
            Logger::log("Hooked resizeBuffers1");
        } else {
            Logger::log("D3D11 Detected");
        }

        presentHookImpl = safetyhook::create_inline(kiero::getMethod<&IDXGISwapChain::Present>(), &presentHook);
        Logger::log("Hooked present");
    } else {
        Logger::log("Failed to initialize Kiero");
    }

    if (auto ptr = FindSignature("41 57 41 56 41 55 41 54 56 57 55 53 48 83 EC ? 44 89 CF 44 89 C3 89 D5 48 89 CE 44 0F B7 A4 24"); ptr) {
        Hook(Mouse2,
            // 1.26.30 alternative
            (void*)ptr
            );
    }
    else {
        //inlined by CLANG
        TrySigHook(Mouse,
            // 1.26.20
            "55 41 57 41 56 41 55 41 54 56 57 53 48 83 EC ? 48 8D 6C 24 ? 48 C7 45 ? ? ? ? ? 44 89 CE 4C 89 C3 49 89 D6",
            // 1.26.10
            "4C 8B DC 53 55 56 57 41 54 41 56 41 57 48 81 EC ? ? ? ? 45 0F B7 E1",
            // 1.21.130
            "4C 8B ? 49 89 ? ? 49 89 ? ? 56 57 41 ? 48 81 EC ? ? ? ? 41 0F ? ? 45 0F"
        );
    }



}