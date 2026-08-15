// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Entry point — COM, core, message loop.
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "Core.h"

namespace {

    struct Com {
        const HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        ~Com() { if (SUCCEEDED(hr)) CoUninitialize(); }
        explicit operator bool() const { return SUCCEEDED(hr); }
    };

#if defined(SHOW_CONSOLE) && SHOW_CONSOLE

    struct Console {
        Console() {
            AllocConsole();
            FILE* f = nullptr;
            freopen_s(&f, "CONOUT$", "w", stdout);
            freopen_s(&f, "CONOUT$", "w", stderr);
            SetConsoleTitleW(L"Spectrum — Debug");
        }
        ~Console() { FreeConsole(); }
    };

#endif

} // namespace

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    Com com;
    if (!com) {
        MessageBoxW(nullptr, L"Failed to initialize COM.", L"Spectrum", MB_OK | MB_ICONERROR);
        return -1;
    }

#if defined(SHOW_CONSOLE) && SHOW_CONSOLE
    Console console;
#endif

    Spectrum::Core app(hInstance);
    if (!app.Initialize()) {
        MessageBoxW(nullptr, L"Failed to initialize.", L"Spectrum", MB_OK | MB_ICONERROR);
        return -1;
    }

    app.Run();
    return 0;
}