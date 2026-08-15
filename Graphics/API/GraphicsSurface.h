#ifndef SPECTRUM_CPP_GRAPHICS_SURFACE_H
#define SPECTRUM_CPP_GRAPHICS_SURFACE_H

#include "Common/Common.h"
#include "Graphics/API/GraphicsHelpers.h"
#include <blend2d.h>
#include <utility>

namespace Spectrum {

    class GraphicsSurface final {
    public:
        GraphicsSurface() = default;
        ~GraphicsSurface() noexcept { Shutdown(); }

        GraphicsSurface(const GraphicsSurface&) = delete;
        GraphicsSurface& operator=(const GraphicsSurface&) = delete;

        [[nodiscard]] bool Initialize(HWND hwnd, bool overlay) {
            Shutdown();
            m_hwnd = hwnd;
            m_overlay = overlay;
            const auto rc = Helpers::Window::GetClientRect(hwnd);
            return CreateBuffer(rc ? Fit(rc->Width()) : 1, rc ? Fit(rc->Height()) : 1);
        }

        void Shutdown() noexcept {
            if (m_drawing) { m_ctx.end(); m_drawing = false; }
            m_image.reset();
            m_buffer.Reset();
            m_hwnd = nullptr;
            m_overlay = false;
            m_width = m_height = 1;
        }

        [[nodiscard]] bool Resize(int w, int h) {
            w = Fit(w);
            h = Fit(h);
            return (w == m_width && h == m_height) || CreateBuffer(w, h);
        }

        [[nodiscard]] bool BeginFrame() {
            m_drawing = m_ctx.begin(m_image) == BL_SUCCESS;
            return m_drawing;
        }

        void Clear(const Color& c) {
            m_ctx.set_comp_op(BL_COMP_OP_SRC_COPY);
            m_ctx.fill_all(ToBL(c));
            m_ctx.set_comp_op(BL_COMP_OP_SRC_OVER);
        }

        [[nodiscard]] bool EndFrame() {
            if (!m_drawing) return true;
            m_ctx.end();
            m_drawing = false;
            return Present();
        }

        [[nodiscard]] BLContext& GetContext() noexcept { return m_ctx; }
        [[nodiscard]] const BLContext& GetContext() const noexcept { return m_ctx; }
        [[nodiscard]] int  GetWidth()  const noexcept { return m_width; }
        [[nodiscard]] int  GetHeight() const noexcept { return m_height; }
        [[nodiscard]] bool IsDrawing() const noexcept { return m_drawing; }
        [[nodiscard]] bool IsOverlay() const noexcept { return m_overlay; }
        [[nodiscard]] HWND GetHwnd()   const noexcept { return m_hwnd; }

    private:
        static int Fit(int v) { return Helpers::Math::Clamp(v, 1, 16384); }

        static BLRgba32 ToBL(const Color& c) noexcept {
            return BLRgba32(FloatToByte(c.r), FloatToByte(c.g), FloatToByte(c.b), FloatToByte(c.a));
        }

        bool CreateBuffer(int w, int h) {
            m_image.reset();
            m_buffer = Helpers::Gdi::CreateAlphaDC(w, h);
            m_width = w;
            m_height = h;
            return m_image.create_from_data(
                w, h, BL_FORMAT_PRGB32, m_buffer.bits, intptr_t(m_buffer.stride),
                BL_DATA_ACCESS_RW) == BL_SUCCESS;
        }

        bool Present() const {
            if (m_overlay) {
                POINT src{ 0, 0 };
                SIZE size{ m_width, m_height };
                BLENDFUNCTION blend{ AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
                return ::UpdateLayeredWindow(
                    m_hwnd, nullptr, nullptr, &size,
                    m_buffer.GetDC(), &src, 0, &blend, ULW_ALPHA) != FALSE;
            }

            HDC dc = ::GetDC(m_hwnd);
            const BOOL ok = ::BitBlt(dc, 0, 0, m_width, m_height, m_buffer.GetDC(), 0, 0, SRCCOPY);
            ::ReleaseDC(m_hwnd, dc);
            return ok != FALSE;
        }

        HWND m_hwnd = nullptr;
        bool m_overlay = false;
        bool m_drawing = false;
        int  m_width = 1;
        int  m_height = 1;

        Helpers::Gdi::AlphaDC m_buffer;
        BLImage   m_image;
        BLContext m_ctx;
    };

} // namespace Spectrum

#endif