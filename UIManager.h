// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Defines the UIManager, which orchestrates the lifecycle, drawing,
// and interaction of all user interface components.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#ifndef SPECTRUM_CPP_UI_MANAGER_H
#define SPECTRUM_CPP_UI_MANAGER_H

#include "Common.h"
#include <memory>
#include <vector>

namespace Spectrum {

    class GraphicsContext;
    class ControllerCore;
    class ColorPicker;

    class UIManager {
    public:
        explicit UIManager(ControllerCore* controller);

        bool Initialize(GraphicsContext& context);
        void RecreateResources(GraphicsContext& context);

        void Draw(GraphicsContext& context);
        bool HandleMouseMessage(UINT msg, int x, int y);

        ColorPicker* GetColorPicker() const { return m_colorPicker.get(); }

    private:
        bool CreateUIComponents(GraphicsContext& context);
        void SetupCallbacks();

        bool HandleMouseMove(int x, int y);
        bool HandleMouseClick(int x, int y);

        ControllerCore* m_controller;
        std::unique_ptr<ColorPicker> m_colorPicker;
    };

}

#endif