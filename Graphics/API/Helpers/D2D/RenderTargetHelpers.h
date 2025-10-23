#ifndef SPECTRUM_CPP_RENDER_TARGET_HELPERS_H
#define SPECTRUM_CPP_RENDER_TARGET_HELPERS_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Render Target Helpers - Safe utilities for Direct2D render target operations
//
// This header provides type-safe utilities for working with different
// Direct2D render target types. It abstracts away the differences between
// ID2D1HwndRenderTarget (normal windows) and ID2D1DCRenderTarget
// (overlay/layered windows).
//
// Key features:
// - Type-safe checking without expensive dynamic_cast
// - Safe access to type-specific methods (e.g., CheckWindowState)
// - Exception-safe operations with optional return types
// - Zero-cost abstractions where possible
//
// Design notes:
// - Uses QueryInterface for type checking instead of dynamic_cast
// - All functions are noexcept to prevent exceptions in rendering pipeline
// - Returns std::optional for operations that may fail
// - Properly manages COM reference counts
//
// Usage example:
//   if (RenderTarget::IsHwndRenderTarget(target)) {
//       auto state = RenderTarget::GetWindowState(target);
//       if (state.has_value() && (state.value() & D2D1_WINDOW_STATE_OCCLUDED)) {
//           // Handle occluded window
//       }
//   }
//
// Version: 1.0
// Architecture pattern: Type-Safe Wrapper
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include <d2d1.h>
#include <optional>

namespace Spectrum::Helpers::RenderTarget {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Type Checking Functions
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    /// Checks if a render target is an ID2D1HwndRenderTarget
    /// 
    /// This function uses COM's QueryInterface to determine the actual
    /// type of the render target without using RTTI or dynamic_cast.
    /// It properly manages reference counts to avoid leaks.
    /// 
    /// @param target Render target to check
    /// @return true if it's a HwndRenderTarget, false otherwise
    inline bool IsHwndRenderTarget(
        ID2D1RenderTarget* target
    ) noexcept
    {
        if (!target) {
            return false;
        }

        ID2D1HwndRenderTarget* hwndTarget = nullptr;
        HRESULT hr = target->QueryInterface(
            __uuidof(ID2D1HwndRenderTarget),
            reinterpret_cast<void**>(&hwndTarget)
        );

        if (SUCCEEDED(hr) && hwndTarget) {
            hwndTarget->Release(); // Don't leak the reference count
            return true;
        }

        return false;
    }

    /// Checks if a render target is an ID2D1DCRenderTarget
    /// 
    /// This function uses COM's QueryInterface to determine the actual
    /// type of the render target. This is the render target type used
    /// for layered/overlay windows.
    /// 
    /// @param target Render target to check
    /// @return true if it's a DCRenderTarget, false otherwise
    inline bool IsDCRenderTarget(
        ID2D1RenderTarget* target
    ) noexcept
    {
        if (!target) {
            return false;
        }

        ID2D1DCRenderTarget* dcTarget = nullptr;
        HRESULT hr = target->QueryInterface(
            __uuidof(ID2D1DCRenderTarget),
            reinterpret_cast<void**>(&dcTarget)
        );

        if (SUCCEEDED(hr) && dcTarget) {
            dcTarget->Release(); // Don't leak the reference count
            return true;
        }

        return false;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Safe Casting Functions
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    /// Safely casts to ID2D1HwndRenderTarget if possible
    /// 
    /// This function first verifies the type using QueryInterface before
    /// performing the cast. This ensures type safety without relying on RTTI.
    /// 
    /// @param target Base render target
    /// @return Pointer to HwndRenderTarget, or nullptr if cast is invalid
    inline ID2D1HwndRenderTarget* AsHwndRenderTarget(
        ID2D1RenderTarget* target
    ) noexcept
    {
        if (!IsHwndRenderTarget(target)) {
            return nullptr;
        }
        return static_cast<ID2D1HwndRenderTarget*>(target);
    }

    /// Safely casts to ID2D1DCRenderTarget if possible
    /// 
    /// This function first verifies the type using QueryInterface before
    /// performing the cast.
    /// 
    /// @param target Base render target
    /// @return Pointer to DCRenderTarget, or nullptr if cast is invalid
    inline ID2D1DCRenderTarget* AsDCRenderTarget(
        ID2D1RenderTarget* target
    ) noexcept
    {
        if (!IsDCRenderTarget(target)) {
            return nullptr;
        }
        return static_cast<ID2D1DCRenderTarget*>(target);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Type-Specific Operations
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    /// Safely gets window state from HwndRenderTarget
    /// 
    /// CheckWindowState is a method specific to ID2D1HwndRenderTarget.
    /// This function safely checks the type before calling it.
    /// 
    /// @param target Render target to query
    /// @return Window state wrapped in optional, or nullopt if not HwndRenderTarget
    inline std::optional<D2D1_WINDOW_STATE> GetWindowState(
        ID2D1RenderTarget* target
    ) noexcept
    {
        auto* hwndTarget = AsHwndRenderTarget(target);
        if (!hwndTarget) {
            return std::nullopt;
        }

        try {
            return hwndTarget->CheckWindowState();
        }
        catch (...) {
            // Catch any potential exceptions from COM
            return std::nullopt;
        }
    }

    /// Checks if window is occluded (only for HwndRenderTarget)
    /// 
    /// This is a convenience function that combines GetWindowState with
    /// a check for the D2D1_WINDOW_STATE_OCCLUDED flag.
    /// 
    /// For DCRenderTarget (overlay windows), this always returns false
    /// since they are topmost and cannot be occluded.
    /// 
    /// @param target Render target to check
    /// @return true if window is occluded, false if visible or not HwndRenderTarget
    inline bool IsWindowOccluded(
        ID2D1RenderTarget* target
    ) noexcept
    {
        auto state = GetWindowState(target);
        if (!state.has_value()) {
            return false; // Not an HWND target (probably overlay), assume not occluded
        }

        return (state.value() & D2D1_WINDOW_STATE_OCCLUDED) != 0;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Validation Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    /// Checks if a render target pointer is valid (not null)
    /// 
    /// @param target Render target to validate
    /// @return true if target is non-null, false otherwise
    inline bool IsValid(
        ID2D1RenderTarget* target
    ) noexcept
    {
        return target != nullptr;
    }

} // namespace Spectrum::Helpers::RenderTarget

#endif // SPECTRUM_CPP_RENDER_TARGET_HELPERS_H