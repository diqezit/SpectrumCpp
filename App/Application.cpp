// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Entry point — COM init, controller lifecycle, top-level error handling.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "ControllerCore.h"
#include <sstream>

namespace {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Debug console (only when SHOW_CONSOLE=1)
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

#if defined(SHOW_CONSOLE) && SHOW_CONSOLE
    void CreateDebugConsole() {
        if (!AllocConsole()) return;

        FILE* f = nullptr;
        freopen_s(&f, "CONOUT$", "w", stdout);
        freopen_s(&f, "CONOUT$", "w", stderr);
        std::cout.clear();
        std::cerr.clear();

        SetConsoleTitleW(L"Spectrum Visualizer — Debug");
    }
#endif

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Error reporting
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    void ShowError(const char* title, const char* message) {
        MessageBoxA(nullptr, message, title, MB_OK | MB_ICONERROR);
    }

    void ShowError(const char* title, const wchar_t* message) {
        MessageBoxW(nullptr, message, title ? nullptr : L"Error", MB_OK | MB_ICONERROR);
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
    // Application lifecycle
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    int RunApplication(HINSTANCE hInstance) {
        // COM is required by D2D, DWrite, WASAPI
        HRESULT hrCom = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(hrCom) && hrCom != RPC_E_CHANGED_MODE) {
            ShowError("Fatal Error",
                L"Failed to initialize COM.\nPlease restart your computer.");
            return -1;
        }

#if defined(SHOW_CONSOLE) && SHOW_CONSOLE
        CreateDebugConsole();
#endif

        LOG_INFO("Application starting...");

        int exitCode = 0;

        try {
            Spectrum::ControllerCore app(hInstance);

            if (!app.Initialize()) {
                ShowError("Initialization Error",
                    "Failed to initialize.\n\n"
                    "Possible causes:\n"
                    "- DirectX 11 not available\n"
                    "- Audio device not found\n"
                    "- Insufficient permissions");
                exitCode = -1;
            }
            else {
                app.Run();
            }
        }
        catch (const std::bad_alloc&) {
            ShowError("Fatal Error", "Out of memory!");
            exitCode = -1;
        }
        catch (const std::exception& e) {
            LOG_ERROR("Unhandled exception: " << e.what());

            std::ostringstream oss;
            oss << "An error occurred:\n\n" << e.what()
                << "\n\nThe application will now close.";
            ShowError("Application Error", oss.str().c_str());
            exitCode = -1;
        }
        catch (...) {
            ShowError("Application Error",
                L"An unknown error occurred.\nThe application will now close.");
            exitCode = -1;
        }

        if (hrCom != RPC_E_CHANGED_MODE)
            CoUninitialize();

        return exitCode;
    }

} // anonymous namespace

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Win32 entry point
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE,
    _In_ LPSTR,
    _In_ int)
{
    return RunApplication(hInstance);
}