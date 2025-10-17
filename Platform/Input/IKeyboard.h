// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Defines the IKeyboard interface, which abstracts the polling of keyboard
// state.
//
// This interface allows the InputManager to be decoupled from the underlying
// platform-specific keyboard API, enabling easier testing and portability by
// adhering to the Dependency Inversion Principle.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef SPECTRUM_CPP_IKEYBOARD_H
#define SPECTRUM_CPP_IKEYBOARD_H

namespace Spectrum::Platform::Input {

    class IKeyboard {
    public:
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // Lifecycle Management
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        virtual ~IKeyboard() = default;

        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // State Queries
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

        [[nodiscard]] virtual bool IsKeyPressed(int virtualKeyCode) const = 0;
    };

} // namespace Spectrum::Platform::Input

#endif // SPECTRUM_CPP_IKEYBOARD_H