// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// This file contains the main entry point of the application.
// It initializes the core controller, handles top-level exceptions, and
// creates a debug console if configured.
//
// Error handling strategy:
// - All exceptions caught at top level
// - Detailed error reporting via message boxes
// - Comprehensive logging for diagnostics
// - Graceful shutdown on errors
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "ControllerCore.h"
#include <sstream>

namespace {

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Build Information
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    constexpr const char* kApplicationName = "Spectrum Visualizer";
    constexpr const char* kVersionMajor = "1";
    constexpr const char* kVersionMinor = "0";
    constexpr const char* kVersionPatch = "0";

#ifdef _DEBUG
    constexpr const char* kBuildConfiguration = "Debug";
#else
    constexpr const char* kBuildConfiguration = "Release";
#endif

#ifdef _WIN64
    constexpr const char* kBuildPlatform = "x64";
#else
    constexpr const char* kBuildPlatform = "x86";
#endif

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Debug Console
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#if defined(SHOW_CONSOLE) && SHOW_CONSOLE
    bool CreateDebugConsole()
    {
        if (!AllocConsole()) {
            return false;
        }

        FILE* stream = nullptr;

        if (freopen_s(&stream, "CONOUT$", "w", stdout) != 0) {
            return false;
        }

        if (freopen_s(&stream, "CONOUT$", "w", stderr) != 0) {
            return false;
        }

        std::cout.clear();
        std::cerr.clear();

        const std::wstring title = std::wstring(L"Spectrum Visualizer - Debug Console [") +
            std::wstring(kBuildConfiguration, kBuildConfiguration + strlen(kBuildConfiguration)) +
            L" " +
            std::wstring(kBuildPlatform, kBuildPlatform + strlen(kBuildPlatform)) +
            L"]";

        SetConsoleTitleW(title.c_str());

        return true;
    }

    void LogBuildInformation()
    {
        LOG_INFO("========================================================");
        LOG_INFO(kApplicationName << " v" << kVersionMajor << "."
            << kVersionMinor << "." << kVersionPatch);
        LOG_INFO("Build: " << kBuildConfiguration << " " << kBuildPlatform);
        LOG_INFO("========================================================");
    }
#endif

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Error Reporting
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    void ReportStdException(const std::exception& e)
    {
        LOG_ERROR("========================================================");
        LOG_ERROR("UNHANDLED EXCEPTION");
        LOG_ERROR("Type: std::exception");
        LOG_ERROR("What: " << e.what());
        LOG_ERROR("========================================================");

        std::ostringstream oss;
        oss << "An error occurred:\n\n"
            << e.what() << "\n\n"
            << "The application will now close.\n"
            << "Check the log file for details.";

        MessageBoxA(
            nullptr,
            oss.str().c_str(),
            "Application Error",
            MB_OK | MB_ICONERROR
        );
    }

    void ReportUnknownException()
    {
        LOG_ERROR("========================================================");
        LOG_ERROR("UNHANDLED EXCEPTION");
        LOG_ERROR("Type: Unknown");
        LOG_ERROR("========================================================");

        MessageBoxW(
            nullptr,
            L"An unknown error occurred.\n\n"
            L"The application will now close.\n"
            L"Check the log file for details.",
            L"Application Error",
            MB_OK | MB_ICONERROR
        );
    }

    void ReportInitializationFailure()
    {
        LOG_ERROR("========================================================");
        LOG_ERROR("INITIALIZATION FAILED");
        LOG_ERROR("========================================================");

        MessageBoxW(
            nullptr,
            L"Failed to initialize the application.\n\n"
            L"Possible causes:\n"
            L"• DirectX 11 not available\n"
            L"• Audio device not found\n"
            L"• Insufficient permissions\n\n"
            L"Check the log file for details.",
            L"Initialization Error",
            MB_OK | MB_ICONERROR
        );
    }

    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    // Application Lifecycle
    // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

    int RunApplication(HINSTANCE hInstance)
    {
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        // CRITICAL: Initialize COM before creating any objects
        // Direct2D, DirectWrite, and WASAPI all require COM
        // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
        HRESULT hrCom = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(hrCom) && hrCom != RPC_E_CHANGED_MODE) {
            MessageBoxW(
                nullptr,
                L"Failed to initialize COM!\n\n"
                L"This is a critical system error.\n"
                L"Please restart your computer.",
                L"Fatal Error",
                MB_OK | MB_ICONERROR
            );
            return -1;
        }

        LOG_INFO("Application: COM initialized successfully");

#if defined(SHOW_CONSOLE) && SHOW_CONSOLE
        if (!CreateDebugConsole()) {
            MessageBoxW(
                nullptr,
                L"Failed to create debug console",
                L"Warning",
                MB_OK | MB_ICONWARNING
            );
        }

        LogBuildInformation();
#endif

        LOG_INFO("Application starting...");

        int exitCode = 0;

        try {
            Spectrum::ControllerCore app(hInstance);

            LOG_INFO("Initializing application...");

            if (!app.Initialize()) {
                ReportInitializationFailure();
                exitCode = -1;
            }
            else {
                LOG_INFO("Initialization successful");
                LOG_INFO("Entering main loop...");

                app.Run();

                LOG_INFO("Application shutdown successful");
            }
        }
        catch (const std::bad_alloc& e) {
            (void)e;
            LOG_ERROR("Memory allocation failed: " << e.what());

            MessageBoxA(
                nullptr,
                "Out of memory!\n\nThe application will now close.",
                "Fatal Error",
                MB_OK | MB_ICONERROR
            );

            exitCode = -1;
        }
        catch (const std::exception& e) {
            ReportStdException(e);
            exitCode = -1;
        }
        catch (...) {
            ReportUnknownException();
            exitCode = -1;
        }

        LOG_INFO("Application terminated normally");

        if (hrCom != RPC_E_CHANGED_MODE) {
            CoUninitialize();
            LOG_INFO("Application: COM uninitialized");
        }

        return exitCode;
    }

} // anonymous namespace

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Entry Point
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Standard Win32 entry point for a GUI application
int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR pCmdLine,
    _In_ int nCmdShow
)
{
    // Suppress unused parameter warnings
    (void)hPrevInstance;
    (void)pCmdLine;
    (void)nCmdShow;

    return RunApplication(hInstance);
}