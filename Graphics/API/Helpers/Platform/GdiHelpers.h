#ifndef SPECTRUM_CPP_GDI_HELPERS_H
#define SPECTRUM_CPP_GDI_HELPERS_H

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// GDI Helpers - RAII wrappers and utilities for GDI resources
//
// This header provides safe, modern C++ wrappers for GDI resources that
// require manual cleanup (DeleteDC, DeleteObject). It follows RAII principles
// to ensure automatic resource cleanup and prevent memory leaks.
//
// Key features:
// - RAII wrappers with custom deleters for unique_ptr
// - Utility functions for creating alpha-compatible bitmaps
// - Safe, exception-safe resource management
// - Compatible with Direct2D DCRenderTarget workflow
//
// Design notes:
// - All deleters are noexcept to prevent exceptions during cleanup
// - Smart pointers ensure deterministic cleanup on scope exit
// - Bitmap bits pointer is optional for performance
// - Top-down DIB format for compatibility with Direct2D coordinate system
//
// Usage example:
//   auto hdc = Gdi::CreateMemoryDC();
//   HBITMAP bitmapHandle;
//   auto fullDC = Gdi::CreateAlphaDC(800, 600, bitmapHandle);
//   // Use fullDC.get() and bitmapHandle...
//   // Automatic cleanup on scope exit
//
// Version: 1.0
// Architecture pattern: RAII Resource Management
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include <windows.h>
#include <memory>

namespace Spectrum::Helpers::Gdi {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Custom Deleters for RAII
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    /// Deleter for HDC objects created with CreateCompatibleDC
    struct DcDeleter {
        void operator()(HDC hdc) const noexcept
        {
            if (hdc) {
                ::DeleteDC(hdc);
            }
        }
    };

    /// Deleter for HBITMAP objects
    struct BitmapDeleter {
        void operator()(HBITMAP bitmap) const noexcept
        {
            if (bitmap) {
                ::DeleteObject(bitmap);
            }
        }
    };

    /// Generic deleter for any HGDIOBJ
    struct GdiObjectDeleter {
        void operator()(HGDIOBJ obj) const noexcept
        {
            if (obj) {
                ::DeleteObject(obj);
            }
        }
    };

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Type Aliases for Smart Pointers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    /// Smart pointer for HDC with automatic cleanup via DeleteDC
    using UniqueDc = std::unique_ptr<
        std::remove_pointer_t<HDC>,
        DcDeleter
    >;

    /// Smart pointer for HBITMAP with automatic cleanup via DeleteObject
    using UniqueBitmap = std::unique_ptr<
        std::remove_pointer_t<HBITMAP>,
        BitmapDeleter
    >;

    /// Smart pointer for generic GDI object with automatic cleanup
    using UniqueGdiObject = std::unique_ptr<
        std::remove_pointer_t<HGDIOBJ>,
        GdiObjectDeleter
    >;

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Core Utility Functions
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    /// Creates a memory DC compatible with the screen
    /// 
    /// This creates a device context that exists only in memory, suitable
    /// for off-screen rendering operations.
    /// 
    /// @return UniqueDc Smart pointer to the created DC, or nullptr on failure
    inline UniqueDc CreateMemoryDC() noexcept
    {
        return UniqueDc(::CreateCompatibleDC(nullptr));
    }

    /// Creates a 32-bit DIB section suitable for per-pixel alpha transparency
    /// 
    /// This function creates a device-independent bitmap with the following
    /// properties:
    /// - 32 bits per pixel (BGRA format)
    /// - Premultiplied alpha channel
    /// - Top-down orientation (negative height)
    /// - Compatible with Direct2D's pixel format expectations
    /// 
    /// @param hdc Device context for the bitmap
    /// @param width Width of the bitmap in pixels
    /// @param height Height of the bitmap in pixels (will be made top-down)
    /// @param bits Optional pointer to receive direct access to bitmap bits
    /// @return HBITMAP handle (not wrapped in smart pointer as it's managed by DC)
    inline HBITMAP CreateAlphaBitmap(
        HDC hdc,
        int width,
        int height,
        void** bits = nullptr
    ) noexcept
    {
        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = -height; // Negative = top-down DIB
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32; // 32-bit for BGRA with alpha
        bmi.bmiHeader.biCompression = BI_RGB;

        return ::CreateDIBSection(
            hdc,
            &bmi,
            DIB_RGB_COLORS,
            bits,
            nullptr,
            0
        );
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // High-Level Convenience Functions
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    /// Creates a fully configured memory DC with an alpha bitmap selected
    /// 
    /// This is a convenience function that combines CreateMemoryDC,
    /// CreateAlphaBitmap, and SelectObject into a single operation.
    /// The bitmap is already selected into the DC when returned.
    /// 
    /// Usage:
    ///   HBITMAP bitmapHandle;
    ///   auto hdc = CreateAlphaDC(800, 600, bitmapHandle);
    ///   // Use hdc.get() for drawing operations
    ///   // bitmapHandle remains valid while hdc is alive
    /// 
    /// @param width Width of the bitmap
    /// @param height Height of the bitmap
    /// @param outBitmap Output parameter for the created bitmap handle
    /// @return UniqueDc with the bitmap already selected, or nullptr on failure
    inline UniqueDc CreateAlphaDC(
        int width,
        int height,
        HBITMAP& outBitmap
    ) noexcept
    {
        auto hdc = CreateMemoryDC();
        if (!hdc) {
            return nullptr;
        }

        outBitmap = CreateAlphaBitmap(hdc.get(), width, height);
        if (!outBitmap) {
            return nullptr;
        }

        ::SelectObject(hdc.get(), outBitmap);
        return hdc;
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Validation Helpers
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    /// Checks if a DC is valid (not null)
    /// 
    /// @param hdc Device context to validate
    /// @return true if DC is non-null, false otherwise
    inline bool IsValid(HDC hdc) noexcept
    {
        return hdc != nullptr;
    }

    /// Checks if a bitmap is valid (not null)
    /// 
    /// @param bitmap Bitmap handle to validate
    /// @return true if bitmap is non-null, false otherwise
    inline bool IsValid(HBITMAP bitmap) noexcept
    {
        return bitmap != nullptr;
    }

} // namespace Spectrum::Helpers::Gdi

#endif // SPECTRUM_CPP_GDI_HELPERS_H