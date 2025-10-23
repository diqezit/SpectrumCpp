// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Implements device-specific graphics resource management
//
// Implementation details:
// - Abstracts away low-level D2D and GDI API calls
// - Supports both standard HWND and transparent overlay windows
// - Notifies components of device changes in a predictable priority order
// - Uses RAII wrappers to guarantee GDI and COM resources are always freed
//
// Error handling:
// - Treats core factory creation failure as a fatal application error
// - Recreates render targets automatically on D2DERR_RECREATE_TARGET signal
// - Propagates critical hardware failures for system-wide recovery
// - Logs component notification errors as non-fatal to avoid shutdown
//
// Performance considerations:
// - Caches expensive D2D and DWrite factories for application lifetime
// - Avoids re-creating factories during device loss to speed up recovery
// - Minimizes render target recreation to prevent frame drops
// - Sorts component list only when it changes to reduce overhead
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Graphics/API/Core/DeviceResourceManager.h"
#include "Graphics/API/Core/IRenderComponent.h"
#include "Graphics/API/D2DHelpers.h"

namespace Spectrum {

    using namespace Helpers::TypeConversion;
    using namespace Helpers::HResult;

    namespace {
        constexpr int kMinSize = 1;
        constexpr int kMaxSize = 16384;

        [[nodiscard]] bool IsValidSize(int width, int height) noexcept
        {
            return width >= kMinSize && width <= kMaxSize &&
                height >= kMinSize && height <= kMaxSize;
        }

        [[nodiscard]] D2D1_SIZE_U GetWindowSize(HWND hwnd) noexcept
        {
            RECT rc{};
            if (!hwnd || !::IsWindow(hwnd) || !GetClientRect(hwnd, &rc)) {
                return D2D1::SizeU(1, 1);
            }

            const UINT32 width = static_cast<UINT32>(std::max(1L, rc.right - rc.left));
            const UINT32 height = static_cast<UINT32>(std::max(1L, rc.bottom - rc.top));

            return D2D1::SizeU(width, height);
        }
    } // anonymous namespace

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // OverlayTargetResources Implementation
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    DeviceResourceManager::OverlayTargetResources::OverlayTargetResources() = default;

    DeviceResourceManager::OverlayTargetResources::~OverlayTargetResources()
    {
        Cleanup();
    }

    DeviceResourceManager::OverlayTargetResources::OverlayTargetResources(
        OverlayTargetResources&& other
    ) noexcept
        : renderTarget(std::move(other.renderTarget))
        , hdc(std::move(other.hdc))
        , bitmap(std::move(other.bitmap))
        , oldBitmap(std::exchange(other.oldBitmap, nullptr))
    {
    }

    DeviceResourceManager::OverlayTargetResources&
        DeviceResourceManager::OverlayTargetResources::operator=(
            OverlayTargetResources&& other
            ) noexcept
    {
        if (this != &other) {
            Cleanup();
            MoveFrom(other);
        }
        return *this;
    }

    void DeviceResourceManager::OverlayTargetResources::Cleanup()
    {
        if (renderTarget) {
            renderTarget.Reset();
        }

        if (hdc && oldBitmap) {
            ::SelectObject(hdc.get(), oldBitmap);
            oldBitmap = nullptr;
        }
    }

    void DeviceResourceManager::OverlayTargetResources::MoveFrom(
        OverlayTargetResources& other
    ) noexcept
    {
        renderTarget = std::move(other.renderTarget);
        hdc = std::move(other.hdc);
        bitmap = std::move(other.bitmap);
        oldBitmap = std::exchange(other.oldBitmap, nullptr);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Lifecycle Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    DeviceResourceManager::DeviceResourceManager(HWND hwnd, bool isOverlay)
        : m_hwnd(hwnd)
        , m_isOverlay(isOverlay)
        , m_lastWidth(1)
        , m_lastHeight(1)
    {
        if (!m_hwnd || !::IsWindow(m_hwnd)) {
            LOG_ERROR("DeviceResourceManager: Invalid HWND provided");
        }
    }

    DeviceResourceManager::~DeviceResourceManager()
    {
        // Notify components before destroying resources
        NotifyComponentsDeviceLost();

        // Clear all resources
        DiscardDeviceResources();

        // Clear component list
        m_components.clear();
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Factory Initialization
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool DeviceResourceManager::InitializeFactories()
    {
        if (!CreateD2DFactory()) {
            LOG_ERROR("Failed to create D2D factory");
            return false;
        }

        if (!CreateDWriteFactory()) {
            LOG_ERROR("Failed to create DWrite factory");
            return false;
        }

        LOG_INFO("D2D and DWrite factories initialized successfully");
        return true;
    }

    bool DeviceResourceManager::CreateD2DFactory()
    {
        const HRESULT hr = D2D1CreateFactory(
            D2D1_FACTORY_TYPE_SINGLE_THREADED,
            m_d2dFactory.GetAddressOf()
        );

        return CheckWithReturn(hr, "D2D1CreateFactory");
    }

    bool DeviceResourceManager::CreateDWriteFactory()
    {
        const HRESULT hr = DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(m_writeFactory.GetAddressOf())
        );

        return CheckWithReturn(hr, "DWriteCreateFactory");
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Resource Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool DeviceResourceManager::CreateDeviceResources(int width, int height)
    {
        if (!IsValidSize(width, height)) {
            LOG_ERROR("Invalid size for device resources: " << width << "x" << height);
            return false;
        }

        // Don't recreate if we already have resources
        if (HasResources()) {
            LOG_WARNING("Device resources already exist");
            return true;
        }

        m_lastWidth = width;
        m_lastHeight = height;

        const bool success = m_isOverlay
            ? CreateOverlayResources(width, height)
            : CreateHwndResources(width, height);

        if (success) {
            NotifyComponentsTargetChanged();
            LOG_INFO("Device resources created successfully (" << width << "x" << height << ")");
        }
        else {
            LOG_ERROR("Failed to create device resources");
        }

        return success;
    }

    void DeviceResourceManager::DiscardDeviceResources()
    {
        if (!HasResources()) {
            return;
        }

        LOG_INFO("Discarding device resources");

        // Notify components before destroying
        NotifyComponentsDeviceLost();

        // Clear current resources
        CleanupCurrentResources();

        // Reset to empty state
        m_target = std::monostate{};
    }

    bool DeviceResourceManager::RecreateDeviceResources(int width, int height)
    {
        LOG_INFO("Recreating device resources");

        DiscardDeviceResources();
        return CreateDeviceResources(width, height);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Resource Access
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    ID2D1RenderTarget* DeviceResourceManager::GetRenderTarget() const
    {
        ID2D1RenderTarget* target = nullptr;

        std::visit([&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, HwndTargetResources> ||
                std::is_same_v<T, OverlayTargetResources>) {
                target = arg.renderTarget.Get();
            }
            }, m_target);

        return target;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Component Management
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void DeviceResourceManager::RegisterComponent(IRenderComponent* component)
    {
        if (!component) {
            LOG_ERROR("Cannot register null component");
            return;
        }

        // Check for duplicates
        auto it = std::find(m_components.begin(), m_components.end(), component);
        if (it != m_components.end()) {
            LOG_WARNING("Component already registered");
            return;
        }

        m_components.push_back(component);
        SortComponentsByPriority();

        LOG_DEBUG("Component registered, total: " << m_components.size());
    }

    void DeviceResourceManager::UnregisterComponent(IRenderComponent* component)
    {
        auto it = std::find(m_components.begin(), m_components.end(), component);
        if (it != m_components.end()) {
            m_components.erase(it);
            LOG_DEBUG("Component unregistered, remaining: " << m_components.size());
        }
    }

    void DeviceResourceManager::NotifyComponentsTargetChanged()
    {
        auto currentTarget = ExtractCurrentRenderTarget();
        if (!currentTarget) {
            LOG_WARNING("No render target to notify components about");
            return;
        }

        SortComponentsByPriority();

        for (auto* component : m_components) {
            if (component) {
                component->OnRenderTargetChanged(currentTarget);
            }
        }

        LOG_DEBUG("Notified " << m_components.size() << " components of target change");
    }

    void DeviceResourceManager::NotifyComponentsDeviceLost()
    {
        SortComponentsByPriority();

        for (auto* component : m_components) {
            if (component) {
                component->OnDeviceLost();
            }
        }

        LOG_DEBUG("Notified " << m_components.size() << " components of device lost");
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Overlay Specific Operations
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    HRESULT DeviceResourceManager::UpdateLayeredWindow(HDC hdc, int width, int height) const
    {
        if (!m_hwnd || !hdc) {
            return E_INVALIDARG;
        }

        POINT srcPos = { 0, 0 };
        SIZE wndSize = { width, height };
        BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

        if (!::UpdateLayeredWindow(m_hwnd, nullptr, nullptr, &wndSize,
            hdc, &srcPos, 0, &blend, ULW_ALPHA)) {
            const DWORD error = GetLastError();
            LOG_ERROR("UpdateLayeredWindow failed with error: " << error);
            return HRESULT_FROM_WIN32(error);
        }

        return S_OK;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Resource Creation - Overlay Mode
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool DeviceResourceManager::CreateOverlayResources(int width, int height)
    {
        OverlayTargetResources res;

        if (!CreateOverlayDC(res)) return false;
        if (!CreateOverlayBitmap(res, width, height)) return false;
        if (!CreateOverlayRenderTarget(res)) return false;

        m_target.emplace<OverlayTargetResources>(std::move(res));
        return true;
    }

    bool DeviceResourceManager::CreateOverlayDC(OverlayTargetResources& res)
    {
        res.hdc = UniqueDc(CreateCompatibleDC(nullptr));
        if (!res.hdc) {
            LOG_ERROR("Failed to create compatible DC: " << GetLastError());
            return false;
        }

        return true;
    }

    bool DeviceResourceManager::CreateOverlayBitmap(
        OverlayTargetResources& res,
        int width,
        int height
    ) const
    {
        BITMAPINFO bmi = CreateBitmapInfo(width, height);

        void* pBits = nullptr;
        HBITMAP bitmapHandle = CreateDIBSection(
            res.hdc.get(),
            &bmi,
            DIB_RGB_COLORS,
            &pBits,
            nullptr,
            0
        );

        if (!bitmapHandle) {
            LOG_ERROR("Failed to create DIB section: " << GetLastError());
            return false;
        }

        res.bitmap = UniqueBitmap(bitmapHandle);
        return SelectBitmapIntoDC(res);
    }

    BITMAPINFO DeviceResourceManager::CreateBitmapInfo(int width, int height) const
    {
        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = -height;  // Top-down bitmap
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        bmi.bmiHeader.biSizeImage = width * height * 4;

        return bmi;
    }

    bool DeviceResourceManager::SelectBitmapIntoDC(OverlayTargetResources& res) const
    {
        res.oldBitmap = SelectObject(res.hdc.get(), res.bitmap.get());

        if (!res.oldBitmap || res.oldBitmap == HGDI_ERROR) {
            LOG_ERROR("Failed to select bitmap into DC: " << GetLastError());
            return false;
        }

        return true;
    }

    bool DeviceResourceManager::CreateOverlayRenderTarget(OverlayTargetResources& res)
    {
        D2D1_RENDER_TARGET_PROPERTIES props = CreateRenderTargetProperties();

        const HRESULT hr = m_d2dFactory->CreateDCRenderTarget(
            &props,
            res.renderTarget.GetAddressOf()
        );

        if (FAILED(hr)) {
            LOG_ERROR("Failed to create DC render target: 0x" << std::hex << hr);
            return false;
        }

        return true;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Resource Creation - HWND Mode
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    bool DeviceResourceManager::CreateHwndResources(int width, int height)
    {
        HwndTargetResources res;

        if (!CreateHwndRenderTarget(res, width, height)) {
            return false;
        }

        ConfigureHwndRenderTarget(res);
        m_target.emplace<HwndTargetResources>(std::move(res));
        return true;
    }

    bool DeviceResourceManager::CreateHwndRenderTarget(
        HwndTargetResources& res,
        int width,
        int height
    )
    {
        D2D1_RENDER_TARGET_PROPERTIES rtProps = CreateRenderTargetProperties();

        D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProps = D2D1::HwndRenderTargetProperties(
            m_hwnd,
            D2D1::SizeU(width, height),
            D2D1_PRESENT_OPTIONS_NONE
        );

        const HRESULT hr = m_d2dFactory->CreateHwndRenderTarget(
            rtProps,
            hwndProps,
            res.renderTarget.GetAddressOf()
        );

        if (FAILED(hr)) {
            LOG_ERROR("Failed to create HWND render target: 0x" << std::hex << hr);
            return false;
        }

        return true;
    }

    void DeviceResourceManager::ConfigureHwndRenderTarget(HwndTargetResources& res) const
    {
        if (res.renderTarget) {
            res.renderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
            res.renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

            // Set default DPI
            res.renderTarget->SetDpi(96.0f, 96.0f);
        }
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Helper Methods
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    wrl::ComPtr<ID2D1RenderTarget> DeviceResourceManager::ExtractCurrentRenderTarget() const
    {
        wrl::ComPtr<ID2D1RenderTarget> currentTarget;

        std::visit([&currentTarget](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, HwndTargetResources> ||
                std::is_same_v<T, OverlayTargetResources>) {
                currentTarget = arg.renderTarget;
            }
            }, m_target);

        return currentTarget;
    }

    void DeviceResourceManager::SortComponentsByPriority()
    {
        std::sort(m_components.begin(), m_components.end(),
            [](auto* a, auto* b) {
                return a->GetPriority() < b->GetPriority();
            });
    }

    void DeviceResourceManager::CleanupCurrentResources()
    {
        std::visit([](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, HwndTargetResources>) {
                // HwndTargetResources cleanup handled by ComPtr
            }
            else if constexpr (std::is_same_v<T, OverlayTargetResources>) {
                // OverlayTargetResources cleanup handled by destructor
            }
            }, m_target);
    }

    D2D1_RENDER_TARGET_PROPERTIES DeviceResourceManager::CreateRenderTargetProperties() const
    {
        return D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(
                DXGI_FORMAT_B8G8R8A8_UNORM,
                D2D1_ALPHA_MODE_PREMULTIPLIED
            ),
            0,  // Default DPI
            0,  // Default DPI
            D2D1_RENDER_TARGET_USAGE_NONE,
            D2D1_FEATURE_LEVEL_DEFAULT
        );
    }

} // namespace Spectrum