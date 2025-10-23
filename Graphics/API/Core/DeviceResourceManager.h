#ifndef SPECTRUM_CPP_DEVICE_RESOURCE_MANAGER_H
#define SPECTRUM_CPP_DEVICE_RESOURCE_MANAGER_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the manager for all volatile graphics resources
//
// Responsibilities:
// - Owns core D2D and DWrite factories for the application
// - Creates and destroys render targets for different window types
// - Manages resource recovery after a graphics device is lost
// - Notifies dependent components about device state changes
//
// Implementation details:
// - Uses std::variant to manage different resource types safely
// - Supports both standard HWND and transparent overlay windows
// - Owns all GDI resources via RAII wrappers for automatic cleanup
// - Encapsulates all platform-specific resource creation logic
//
// Resource lifecycle:
// - Factories are created once and persist for application lifetime
// - Render targets are created on demand or after device loss
// - Components are notified immediately after resource changes
// - Destructor handles final cleanup in the correct order
//
// Thread safety:
// - This class is not thread-safe by design
// - All access must be synchronized externally by a controller
// - Assumes all operations occur on the main rendering thread
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Common/Common.h"
#include "Graphics/API/Helpers/Platform/GdiHelpers.h"
#include <variant>
#include <vector>

namespace Spectrum {

    using namespace Helpers::Gdi;

    class IRenderComponent;

    class DeviceResourceManager final
    {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Resource Structures
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        struct HwndTargetResources {
            wrl::ComPtr<ID2D1HwndRenderTarget> renderTarget;
        };

        struct OverlayTargetResources {
            wrl::ComPtr<ID2D1DCRenderTarget> renderTarget;
            UniqueDc     hdc;
            UniqueBitmap bitmap;
            HGDIOBJ      oldBitmap = nullptr;

            OverlayTargetResources();
            ~OverlayTargetResources();

            OverlayTargetResources(OverlayTargetResources&& other) noexcept;
            OverlayTargetResources& operator=(OverlayTargetResources&& other) noexcept;

            OverlayTargetResources(const OverlayTargetResources&) = delete;
            OverlayTargetResources& operator=(const OverlayTargetResources&) = delete;

        private:
            void Cleanup();
            void MoveFrom(OverlayTargetResources& other) noexcept;
        };

        using ResourceVariant = std::variant<
            std::monostate,
            HwndTargetResources,
            OverlayTargetResources
        >;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        DeviceResourceManager(HWND hwnd, bool isOverlay);
        ~DeviceResourceManager();

        DeviceResourceManager(const DeviceResourceManager&) = delete;
        DeviceResourceManager& operator=(const DeviceResourceManager&) = delete;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Factory Initialization
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool InitializeFactories();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Resource Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool CreateDeviceResources(int width, int height);
        void DiscardDeviceResources();
        [[nodiscard]] bool RecreateDeviceResources(int width, int height);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Resource Access
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] ID2D1RenderTarget* GetRenderTarget() const;
        [[nodiscard]] ID2D1Factory* GetD2DFactory() const { return m_d2dFactory.Get(); }
        [[nodiscard]] IDWriteFactory* GetDWriteFactory() const { return m_writeFactory.Get(); }
        [[nodiscard]] const ResourceVariant& GetResources() const { return m_target; }
        [[nodiscard]] ResourceVariant& GetResources() { return m_target; }
        [[nodiscard]] bool HasResources() const { return !std::holds_alternative<std::monostate>(m_target); }

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Component Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        void RegisterComponent(IRenderComponent* component);
        void UnregisterComponent(IRenderComponent* component);
        void NotifyComponentsTargetChanged();
        void NotifyComponentsDeviceLost();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Overlay Specific Operations
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] HRESULT UpdateLayeredWindow(HDC hdc, int width, int height) const;

    private:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Factory Creation
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool CreateD2DFactory();
        [[nodiscard]] bool CreateDWriteFactory();

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Resource Creation - Overlay Mode
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool CreateOverlayResources(int width, int height);
        [[nodiscard]] bool CreateOverlayDC(OverlayTargetResources& res);
        [[nodiscard]] bool CreateOverlayBitmap(OverlayTargetResources& res, int width, int height) const;
        [[nodiscard]] BITMAPINFO CreateBitmapInfo(int width, int height) const;
        [[nodiscard]] bool SelectBitmapIntoDC(OverlayTargetResources& res) const;
        [[nodiscard]] bool CreateOverlayRenderTarget(OverlayTargetResources& res);

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Resource Creation - HWND Mode
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] bool CreateHwndResources(int width, int height);
        [[nodiscard]] bool CreateHwndRenderTarget(HwndTargetResources& res, int width, int height);
        void ConfigureHwndRenderTarget(HwndTargetResources& res) const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Helper Methods
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] wrl::ComPtr<ID2D1RenderTarget> ExtractCurrentRenderTarget() const;
        void SortComponentsByPriority();
        void CleanupCurrentResources();
        [[nodiscard]] D2D1_RENDER_TARGET_PROPERTIES CreateRenderTargetProperties() const;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Member Variables
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        HWND m_hwnd;                                    // Target window handle
        bool m_isOverlay;                               // Rendering mode flag

        wrl::ComPtr<ID2D1Factory> m_d2dFactory;         // Direct2D factory
        wrl::ComPtr<IDWriteFactory> m_writeFactory;     // DirectWrite factory

        ResourceVariant m_target;                       // Current render target resources
        std::vector<IRenderComponent*> m_components;    // Registered components

        int m_lastWidth;                                // Last known width
        int m_lastHeight;                               // Last known height
    };

} // namespace Spectrum

#endif // SPECTRUM_CPP_DEVICE_RESOURCE_MANAGER_H