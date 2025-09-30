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

// DLL entry point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    // Skip hooking in Detours helper processes
    if (DetourIsHelperProcess()) {
        return TRUE;
    }

    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            OutputDebugStringA("[AffinityHook] DLL loaded, installing hook...");

            // Install the hook
            DetourRestoreAfterWith();
            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());
            DetourAttach(reinterpret_cast<PVOID*>(&TrueSetProcessAffinityMask), reinterpret_cast<PVOID>(HookedSetProcessAffinityMask));

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