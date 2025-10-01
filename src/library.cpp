#include "library.h"
#include <windows.h>
#include <stdio.h>

// Include appropriate Detours header based on architecture
#if defined(_M_X64) || defined(__x86_64__)
    #include "detours_x64.h"
#else
    #include "detours_x86.h"
#endif

// Dummy export function for DLL injectors that require at least one export
extern "C" __declspec(dllexport) void DummyExport() {
    // This function exists solely to satisfy DLL injectors
    // It is never called
}

// Function pointer to the original SetProcessAffinityMask
static BOOL (WINAPI* TrueSetProcessAffinityMask)(HANDLE hProcess, DWORD_PTR dwProcessAffinityMask) = SetProcessAffinityMask;
static BOOL (WINAPI* TrueFreeLibrary)(HMODULE hLibModule) = FreeLibrary;

// Hooked version of SetProcessAffinityMask
BOOL WINAPI HookedSetProcessAffinityMask(HANDLE hProcess, DWORD_PTR dwProcessAffinityMask) {
    char logBuffer[256];

    // Log the interception with original mask
    sprintf_s(logBuffer, sizeof(logBuffer),
              "[AffinityHook] Intercepted SetProcessAffinityMask call - Original mask: 0x%llX",
              static_cast<unsigned long long>(dwProcessAffinityMask));
    OutputDebugStringA(logBuffer);

    // Override the affinity mask to use all cores
    DWORD_PTR newMask = 0xFFFFFFFFFFFFFFFF;

    sprintf_s(logBuffer, sizeof(logBuffer),
              "[AffinityHook] Modifying mask to: 0x%llX (all cores)",
              static_cast<unsigned long long>(newMask));
    OutputDebugStringA(logBuffer);

    // Call the original function with modified mask
    return TrueSetProcessAffinityMask(hProcess, newMask);
}

static HMODULE g_hModule = nullptr;

BOOL WINAPI HookedFreeLibrary(HMODULE hModule)
{
    OutputDebugStringA("[AffinityHook] Intercepted FreeLibrary call");
    if (g_hModule != nullptr && g_hModule == hModule) {
        OutputDebugStringA("[AffinityHook] Preventing unload of my module");
        return TRUE; // pretend success, but do not unload
    }
    return TrueFreeLibrary(hModule);
}

// DLL entry point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, [[maybe_unused]] LPVOID lpvReserved) {
    // Skip hooking in Detours helper processes
    if (DetourIsHelperProcess()) {
        return TRUE;
    }

    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            OutputDebugStringA("[AffinityHook] DLL loaded, installing hook...");
            DisableThreadLibraryCalls(hinstDLL);

            g_hModule = hinstDLL;

            // Prevent DLL from being unloaded by incrementing reference count
            HMODULE hModule;
            if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                                   reinterpret_cast<LPCWSTR>(hinstDLL), &hModule)) {
                OutputDebugStringA("[AffinityHook] DLL pinned in memory");
            } else {
                OutputDebugStringA("[AffinityHook] WARNING: Failed to pin DLL in memory");
            }

            // Install the hook
            DetourRestoreAfterWith();
            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());
            DetourAttach(reinterpret_cast<PVOID*>(&TrueSetProcessAffinityMask), reinterpret_cast<PVOID>(HookedSetProcessAffinityMask));
            DetourAttach(reinterpret_cast<PVOID*>(&TrueFreeLibrary), reinterpret_cast<PVOID>(HookedFreeLibrary));

            if (DetourTransactionCommit() == NO_ERROR) {
                OutputDebugStringA("[AffinityHook] Hook installed successfully");
            } else {
                OutputDebugStringA("[AffinityHook] ERROR: Hook installation failed");
                return FALSE;
            }
            break;

        case DLL_PROCESS_DETACH:
            OutputDebugStringA("[AffinityHook] DLL unloading, removing hook...");

            // Remove the hook
            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());
            DetourDetach(reinterpret_cast<PVOID*>(&TrueSetProcessAffinityMask), reinterpret_cast<PVOID>(HookedSetProcessAffinityMask));
            DetourDetach(reinterpret_cast<PVOID*>(&TrueFreeLibrary), reinterpret_cast<PVOID>(HookedFreeLibrary));

            if (DetourTransactionCommit() == NO_ERROR) {
                OutputDebugStringA("[AffinityHook] Hook removed successfully");
            } else {
                OutputDebugStringA("[AffinityHook] ERROR: Hook removal failed");
            }
            break;

        default:
            OutputDebugStringA("[AffinityHook] DLL loaded, but not attaching hook. Reason: unknown fdwReason");
            return FALSE;
    }

    return TRUE;
}