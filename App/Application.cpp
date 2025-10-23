// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file contains the main entry point of the application.
// It initializes the core controller, handles top-level exceptions, and
// creates a debug console if configured.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "ControllerCore.h"
#include "Common/PlatformUtils.h" 

namespace {

    // user can enable console for debugging via preprocessor define
#if defined(SHOW_CONSOLE) && SHOW_CONSOLE
    void CreateDebugConsole() {
        if (!AllocConsole()) return;

        FILE* stream = nullptr;
        freopen_s(&stream, "CONOUT$", "w", stdout);
        freopen_s(&stream, "CONOUT$", "w", stderr);

        std::cout.clear();
        std::cerr.clear();

        SetConsoleTitleW(L"Spectrum Visualizer - Debug Console");
    }
#endif

    void ReportStdException(const std::exception& e) {
        LOG_ERROR("Unhandled exception: " << e.what());
        MessageBoxA(
            nullptr,
            e.what(),
            "Unhandled Exception",
            MB_OK | MB_ICONERROR
        );
    }

    void ReportUnknownException() {
        LOG_ERROR("Unknown unhandled exception");
        MessageBoxW(
            nullptr,
            L"An unknown error occurred",
            L"Unhandled Exception",
            MB_OK | MB_ICONERROR
        );
    }

    // encapsulates the main application logic in a try-catch block
    // this ensures any unhandled exception is caught and reported to the user
    int RunApplication(HINSTANCE hInstance) {
#if defined(SHOW_CONSOLE) && SHOW_CONSOLE
        CreateDebugConsole();
#endif

        try {
            Spectrum::ControllerCore app(hInstance);
            if (!app.Initialize()) {
                LOG_ERROR("Failed to initialize application");
                MessageBoxW(
                    nullptr,
                    L"Application failed to initialize",
                    L"Error",
                    MB_OK | MB_ICONERROR
                );
                return -1;
            }
            app.Run();
        }
        catch (const std::exception& e) {
            ReportStdException(e);
            return -1;
        }
        catch (...) {
            ReportUnknownException();
            return -1;
        }

        return 0;
    }

} // anonymous namespace

// standard Win32 entry point for a GUI application
int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ PWSTR pCmdLine,
    _In_ int nCmdShow
) {
    (void)hPrevInstance;
    (void)pCmdLine;
    (void)nCmdShow;

    return RunApplication(hInstance);
}