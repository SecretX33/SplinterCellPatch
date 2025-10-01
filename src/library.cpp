#include "library.h"
#include <windows.h>
#include <stdio.h>

#if defined(_M_X64) || defined(__x86_64__)
    #include "detours_x64.h"
#else
    #include "detours_x86.h"
#endif

// Dummy export function for DLL injectors that require at least one export
extern "C" __declspec(dllexport) void DummyExport() {
    // This function exists solely to satisfy DLL injectors
    // It is never called
    OutputDebugStringA("[AffinityHook] DummyExport called - this should never happen!");
}

static HMODULE g_hModule = nullptr;

// Pointers to the original functions
typedef BOOL (WINAPI *PFN_SetProcessAffinityMask)(HANDLE, DWORD_PTR);
static PFN_SetProcessAffinityMask Real_SetProcessAffinityMask = nullptr;

typedef BOOL (WINAPI *PFN_FreeLibrary)(HMODULE hModule);
static PFN_FreeLibrary Real_FreeLibrary = nullptr;

// Hooked version of SetProcessAffinityMask
BOOL WINAPI Hooked_SetProcessAffinityMask(HANDLE hProcess, DWORD_PTR dwProcessAffinityMask) {
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
    return Real_SetProcessAffinityMask(hProcess, newMask);
}

BOOL WINAPI Hooked_FreeLibrary(HMODULE hModule)
{
    OutputDebugStringA("[AffinityHook] Intercepted FreeLibrary call");
    if (g_hModule != nullptr && g_hModule == hModule) {
        OutputDebugStringA("[AffinityHook] Preventing unload of my module");
        return TRUE; // pretend success, but do not unload
    }
    return Real_FreeLibrary(hModule);
}

bool LoadFunctionReferences() {
    OutputDebugStringA("[AffinityHook] Loading references to original functions...");

    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    if (!hKernel32) {
        OutputDebugStringA("[AffinityHook] GetModuleHandleA(kernel32) failed");
        return false;
    }

    Real_SetProcessAffinityMask = reinterpret_cast<PFN_SetProcessAffinityMask>(GetProcAddress(hKernel32, "SetProcessAffinityMask"));
    if (!Real_SetProcessAffinityMask) {
        OutputDebugStringA("[AffinityHook] GetProcAddress(SetProcessAffinityMask) failed");
        return false;
    }

    Real_FreeLibrary = reinterpret_cast<PFN_FreeLibrary>(GetProcAddress(hKernel32, "FreeLibrary"));
    if (!Real_FreeLibrary) {
        OutputDebugStringA("[AffinityHook] GetProcAddress(FreeLibrary) failed");
        return false;
    }

    return true;
}

BOOL InstallHook() {
    DWORD error = NO_ERROR;

    error = DetourRestoreAfterWith();
    if (error != NO_ERROR) {
        OutputDebugStringA("[AffinityHook] DetourRestoreAfterWith failed");
        return FALSE;
    }

    error = DetourTransactionBegin();
    if (error != NO_ERROR) {
        OutputDebugStringA("[AffinityHook] DetourTransactionBegin failed");
        return FALSE;
    }
    error = DetourUpdateThread(GetCurrentThread());
    if (error != NO_ERROR) {
        OutputDebugStringA("[AffinityHook] DetourUpdateThread failed");
        DetourTransactionAbort();
        return FALSE;
    }

    error = DetourAttach(reinterpret_cast<PVOID*>(&Real_SetProcessAffinityMask), reinterpret_cast<PVOID>(Hooked_SetProcessAffinityMask));
    if (error == NO_ERROR) error = DetourAttach(reinterpret_cast<PVOID*>(&Real_FreeLibrary), reinterpret_cast<PVOID>(Hooked_FreeLibrary));

    if (error != NO_ERROR) {
        OutputDebugStringA("[AffinityHook] DetourAttach failed");
        DetourTransactionAbort();
        return FALSE;
    }

    error = DetourTransactionCommit();
    if (error != NO_ERROR) {
        OutputDebugStringA("[AffinityHook] ERROR: Hook installation failed");
        return FALSE;
    }

    OutputDebugStringA("[AffinityHook] Hook installed successfully");
    return TRUE;
}

BOOL UninstallHook() {
    DWORD error = NO_ERROR;
    error = DetourTransactionBegin();
    if (error != NO_ERROR) {
        OutputDebugStringA("[AffinityHook] DetourTransactionBegin failed");
        return FALSE;
    }

    error = DetourUpdateThread(GetCurrentThread());
    if (error != NO_ERROR) {
        OutputDebugStringA("[AffinityHook] DetourUpdateThread failed");
        DetourTransactionAbort();
        return FALSE;
    }

    error = DetourDetach(reinterpret_cast<PVOID*>(&Real_SetProcessAffinityMask), reinterpret_cast<PVOID>(Hooked_SetProcessAffinityMask));
    if (error == NO_ERROR) DetourDetach(reinterpret_cast<PVOID*>(&Real_FreeLibrary), reinterpret_cast<PVOID>(Hooked_FreeLibrary));

    if (error != NO_ERROR) {
        OutputDebugStringA("[AffinityHook] DetourDetach failed");
        DetourTransactionAbort();
        return FALSE;
    }

    error = DetourTransactionCommit();
    if (error != NO_ERROR) {
        OutputDebugStringA("[AffinityHook] ERROR: Hook uninstall failed");
        return FALSE;
    }

    OutputDebugStringA("[AffinityHook] Hook uninstalled successfully");
    return TRUE;
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

            if (!LoadFunctionReferences()) {
                return FALSE;
            }

            // Prevent DLL from being unloaded by incrementing reference count
            HMODULE hModule;
            if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                                   reinterpret_cast<LPCWSTR>(hinstDLL), &hModule)) {
                OutputDebugStringA("[AffinityHook] DLL pinned in memory");
            } else {
                OutputDebugStringA("[AffinityHook] WARNING: Failed to pin DLL in memory");
            }

            if (!InstallHook()) {
                return FALSE;
            }
            break;

        case DLL_PROCESS_DETACH:
            OutputDebugStringA("[AffinityHook] DLL unloading, removing hook...");

            if (!UninstallHook()) {
                return FALSE;
            }
            break;

        default:
            OutputDebugStringA("[AffinityHook] DLL loaded, but not attaching hook. Reason: unknown fdwReason");
            return FALSE;
    }

    return TRUE;
}