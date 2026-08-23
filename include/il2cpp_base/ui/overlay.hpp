#pragma once

#include <d3d11.h>
#include <dxgi.h>

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <atomic>
#include <functional>
#include <mutex>

#include "il2cpp_base/core/config.hpp"
#include "il2cpp_base/core/hooks.hpp"
#include "il2cpp_base/core/logger.hpp"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace il2cpp_base::ui {

using RenderCallback = std::function<void()>;

class Overlay {
public:
    static Overlay& instance();

    bool init();
    void shutdown();

    void set_render_callback(RenderCallback cb);
    bool menu_visible() const { return menu_visible_.load(); }
    void set_menu_visible(bool visible) { menu_visible_.store(visible); }

private:
    Overlay() = default;

    static HRESULT WINAPI hooked_present(IDXGISwapChain* swap, UINT sync, UINT flags);
    static LRESULT WINAPI hooked_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    bool setup_imgui(IDXGISwapChain* swap);
    void recreate_rtv(IDXGISwapChain* swap);
    void render_frame(IDXGISwapChain* swap);
    void request_main_thread_pump();

    static constexpr UINT kMainThreadPumpMsg = WM_APP + 0x4D01;

    using present_fn = HRESULT(WINAPI*)(IDXGISwapChain*, UINT, UINT);
    using wndproc_fn = LRESULT(WINAPI*)(HWND, UINT, WPARAM, LPARAM);

    present_fn original_present_ = nullptr;
    wndproc_fn original_wndproc_ = nullptr;

    HWND hwnd_ = nullptr;
    ID3D11Device* device_ = nullptr;
    ID3D11DeviceContext* context_ = nullptr;
    ID3D11RenderTargetView* rtv_ = nullptr;
    UINT buffer_width_ = 0;
    UINT buffer_height_ = 0;

    std::mutex callback_mutex_;
    RenderCallback render_callback_;
    std::atomic<bool> menu_visible_{ false };
    std::atomic<bool> imgui_ready_{ false };
    std::atomic<bool> initialized_{ false };
    std::atomic<bool> pump_pending_{ false };
};

} // namespace il2cpp_base::ui
