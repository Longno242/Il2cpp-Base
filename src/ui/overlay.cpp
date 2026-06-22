#include "il2cpp_base/ui/overlay.hpp"

#include <MinHook.h>

namespace il2cpp_base::ui {

Overlay& Overlay::instance() {
    static Overlay overlay;
    return overlay;
}

void Overlay::set_render_callback(RenderCallback cb) {
    std::lock_guard lock(callback_mutex_);
    render_callback_ = std::move(cb);
}

bool Overlay::init() {
    if (initialized_.load()) {
        return true;
    }

    if (!hooks::HookManager::instance().init()) {
        return false;
    }

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"Il2cppBaseDummy";
    RegisterClassExW(&wc);

    HWND dummy = CreateWindowExW(
        0, wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW,
        0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);

    DXGI_SWAP_CHAIN_DESC desc{};
    desc.BufferCount = 1;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow = dummy;
    desc.SampleDesc.Count = 1;
    desc.Windowed = TRUE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    IDXGISwapChain* swap = nullptr;
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    D3D_FEATURE_LEVEL level{};
    const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };

    if (FAILED(D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            levels, 1, D3D11_SDK_VERSION, &desc,
            &swap, &device, &level, &context))) {
        DestroyWindow(dummy);
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        LOG_ERROR("Overlay", "Failed to create dummy D3D11 device");
        return false;
    }

    void** vtable = *reinterpret_cast<void***>(swap);
    void* present_target = vtable[8];

    swap->Release();
    device->Release();
    context->Release();
    DestroyWindow(dummy);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    if (!hooks::HookManager::instance().create("DXGI.Present", present_target,
                                               &Overlay::hooked_present, &original_present_)) {
        return false;
    }

    if (!hooks::HookManager::instance().enable("DXGI.Present")) {
        return false;
    }

    initialized_.store(true);
    LOG_INFO("Overlay", "DXGI Present hook installed");
    return true;
}

void Overlay::shutdown() {
    if (!initialized_.load()) {
        return;
    }

    hooks::HookManager::instance().disable("DXGI.Present");
    hooks::HookManager::instance().remove("DXGI.Present");

    if (imgui_ready_.load()) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        if (rtv_) {
            rtv_->Release();
            rtv_ = nullptr;
        }
        if (context_) {
            context_->Release();
            context_ = nullptr;
        }
        if (device_) {
            device_->Release();
            device_ = nullptr;
        }

        if (hwnd_ && original_wndproc_) {
            SetWindowLongPtrW(hwnd_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(original_wndproc_));
        }

        imgui_ready_.store(false);
    }

    initialized_.store(false);
}

bool Overlay::setup_imgui(IDXGISwapChain* swap) {
    if (FAILED(swap->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&device_)))) {
        LOG_ERROR("Overlay", "GetDevice failed");
        return false;
    }

    device_->GetImmediateContext(&context_);

    DXGI_SWAP_CHAIN_DESC sc_desc{};
    swap->GetDesc(&sc_desc);
    hwnd_ = sc_desc.OutputWindow;

    ID3D11Texture2D* back_buffer = nullptr;
    if (FAILED(swap->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back_buffer)))) {
        LOG_ERROR("Overlay", "GetBuffer failed");
        return false;
    }

    device_->CreateRenderTargetView(back_buffer, nullptr, &rtv_);
    back_buffer->Release();

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding  = 4.0f;
    style.GrabRounding   = 4.0f;

    ImGui_ImplWin32_Init(hwnd_);
    ImGui_ImplDX11_Init(device_, context_);

    original_wndproc_ = reinterpret_cast<wndproc_fn>(
        SetWindowLongPtrW(hwnd_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&Overlay::hooked_wndproc)));

    imgui_ready_.store(true);
    LOG_INFO("Overlay", "ImGui initialized (HWND %p)", hwnd_);
    return true;
}

void Overlay::render_frame(IDXGISwapChain* swap) {
    if (!imgui_ready_.load() && !setup_imgui(swap)) {
        return;
    }

    if (GetAsyncKeyState(config::kMenuToggleKey) & 1) {
        menu_visible_.store(!menu_visible_.load());
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (menu_visible_.load()) {
        ImGui::SetNextWindowSize(ImVec2(420, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Il2cppBase", nullptr, ImGuiWindowFlags_NoCollapse)) {
            RenderCallback cb;
            {
                std::lock_guard lock(callback_mutex_);
                cb = render_callback_;
            }
            if (cb) {
                cb();
            }
        }
        ImGui::End();
    }

    ImGui::Render();

    context_->OMSetRenderTargets(1, &rtv_, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

HRESULT WINAPI Overlay::hooked_present(IDXGISwapChain* swap, UINT sync, UINT flags) {
    auto& self = instance();
    if (self.initialized_.load()) {
        self.render_frame(swap);
    }
    return self.original_present_(swap, sync, flags);
}

LRESULT WINAPI Overlay::hooked_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    auto& self = instance();

    if (self.menu_visible_.load() && ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
        return TRUE;
    }

    return CallWindowProcW(self.original_wndproc_, hwnd, msg, wparam, lparam);
}

} // namespace il2cpp_base::ui
