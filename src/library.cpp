#include "library.h"
#include <windows.h>
#include <format>
#include <string>

#if defined(_M_X64) || defined(__x86_64__)
#include "detours_x64.h"
#else
#include "detours_x86.h"
#endif

inline constexpr DWORD_PTR ALL_CORES_MASK = 0xFFFFFFFFFFFFFFFF;

// Dummy export function for DLL injectors that require at least one export
extern "C" __declspec(dllexport) void DummyExport() {
    // This function exists solely to satisfy DLL injectors
    // It is never called
    OutputDebugStringA("[AffinityHook] DummyExport called - this should never happen!");
}

static HMODULE g_hModule = nullptr;

typedef BOOL (WINAPI *PFN_SetProcessAffinityMask)(HANDLE, DWORD_PTR);
static PFN_SetProcessAffinityMask Real_SetProcessAffinityMask = nullptr;

typedef BOOL (WINAPI *PFN_FreeLibrary)(HMODULE hModule);
static PFN_FreeLibrary Real_FreeLibrary = nullptr;

BOOL WINAPI Hooked_SetProcessAffinityMask(HANDLE hProcess, DWORD_PTR dwProcessAffinityMask) {
    if (hProcess == nullptr || hProcess == INVALID_HANDLE_VALUE) {
        OutputDebugStringA("[AffinityHook] Invalid hProcess handle detected");
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    // Preserve caller's error state
    DWORD lastError = GetLastError();

    // Log the interception with original mask
    std::string logMsg = std::format(
        "[AffinityHook] Intercepted SetProcessAffinityMask call - Original mask: 0x{:X}",
        dwProcessAffinityMask
    );
    OutputDebugStringA(logMsg.c_str());

    // Override the affinity mask to use all cores
    logMsg = std::format(
        "[AffinityHook] Modifying mask to: 0x{:X} (all cores)",
        ALL_CORES_MASK
    );
    OutputDebugStringA(logMsg.c_str());

    // Restore error state before calling original function
    SetLastError(lastError);

    // Call the original function with modified mask
    return Real_SetProcessAffinityMask(hProcess, ALL_CORES_MASK);
}

BOOL WINAPI Hooked_FreeLibrary(HMODULE hModule) {
    OutputDebugStringA("[AffinityHook] Intercepted FreeLibrary call");
    if (g_hModule != nullptr && g_hModule == hModule) {
        OutputDebugStringA("[AffinityHook] Preventing unload of my module");
        SetLastError(ERROR_SUCCESS);
        return TRUE; // pretend success, but do not unload
    }
    return Real_FreeLibrary(hModule);
}

[[nodiscard]] bool LoadFunctionReferences() {
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

[[nodiscard]] bool InstallHook() {
    if (!Real_SetProcessAffinityMask || !Real_FreeLibrary) {
        OutputDebugStringA("[AffinityHook] ERROR: Function pointers not initialized");
        return false;
    }

    DWORD error = NO_ERROR;

    error = DetourRestoreAfterWith();
    if (error != NO_ERROR) {
        std::string errorMsg = std::format("[AffinityHook] DetourRestoreAfterWith failed with error: 0x{:X}", error);
        OutputDebugStringA(errorMsg.c_str());
        return false;
    }

    error = DetourTransactionBegin();
    if (error != NO_ERROR) {
        std::string errorMsg = std::format("[AffinityHook] DetourTransactionBegin failed with error: 0x{:X}", error);
        OutputDebugStringA(errorMsg.c_str());
        return false;
    }
    error = DetourUpdateThread(GetCurrentThread());
    if (error != NO_ERROR) {
        std::string errorMsg = std::format("[AffinityHook] DetourUpdateThread failed with error: 0x{:X}", error);
        OutputDebugStringA(errorMsg.c_str());
        DetourTransactionAbort();
        return false;
    }

    error = DetourAttach(reinterpret_cast<PVOID *>(&Real_SetProcessAffinityMask), reinterpret_cast<PVOID>(Hooked_SetProcessAffinityMask));
    if (error == NO_ERROR) error = DetourAttach(reinterpret_cast<PVOID *>(&Real_FreeLibrary), reinterpret_cast<PVOID>(Hooked_FreeLibrary));

    if (error != NO_ERROR) {
        std::string errorMsg = std::format("[AffinityHook] DetourAttach failed with error: 0x{:X}", error);
        OutputDebugStringA(errorMsg.c_str());
        DetourTransactionAbort();
        return false;
    }

    error = DetourTransactionCommit();
    if (error != NO_ERROR) {
        std::string errorMsg = std::format("[AffinityHook] ERROR: Hook installation failed with error: 0x{:X}", error);
        OutputDebugStringA(errorMsg.c_str());
        return false;
    }

    OutputDebugStringA("[AffinityHook] Hook installed successfully");
    return true;
}

[[nodiscard]] bool UninstallHook() {
    DWORD error = NO_ERROR;

    error = DetourTransactionBegin();
    if (error != NO_ERROR) {
        std::string errorMsg = std::format("[AffinityHook] DetourTransactionBegin failed with error: 0x{:X}", error);
        OutputDebugStringA(errorMsg.c_str());
        return false;
    }

    error = DetourUpdateThread(GetCurrentThread());
    if (error != NO_ERROR) {
        std::string errorMsg = std::format("[AffinityHook] DetourUpdateThread failed with error: 0x{:X}", error);
        OutputDebugStringA(errorMsg.c_str());
        DetourTransactionAbort();
        return false;
    }

    error = DetourDetach(reinterpret_cast<PVOID *>(&Real_SetProcessAffinityMask), reinterpret_cast<PVOID>(Hooked_SetProcessAffinityMask));
    if (error == NO_ERROR) {
        error = DetourDetach(reinterpret_cast<PVOID *>(&Real_FreeLibrary), reinterpret_cast<PVOID>(Hooked_FreeLibrary));
    }

    if (error != NO_ERROR) {
        std::string errorMsg = std::format("[AffinityHook] DetourDetach failed with error: 0x{:X}", error);
        OutputDebugStringA(errorMsg.c_str());
        DetourTransactionAbort();
        return false;
    }

    error = DetourTransactionCommit();
    if (error != NO_ERROR) {
        std::string errorMsg = std::format("[AffinityHook] ERROR: Hook uninstall failed with error: 0x{:X}", error);
        OutputDebugStringA(errorMsg.c_str());
        return false;
    }

    OutputDebugStringA("[AffinityHook] Hook uninstalled successfully");
    return true;
}

bool PinDllToMemory(LPCWSTR lpModuleName) {
    // Prevent DLL from being unloaded by incrementing reference count
    HMODULE hModule;
    const BOOL success = GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
        lpModuleName, &hModule
    );
    if (!success) {
        DWORD error = GetLastError();
        std::string errorMsg = std::format("[AffinityHook] CRITICAL: Failed to pin DLL in memory (error: 0x{:X})", error);
        OutputDebugStringA(errorMsg.c_str());
        return false;
    }
    OutputDebugStringA("[AffinityHook] DLL pinned in memory");

    return true;
}

// DLL entry point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, [[maybe_unused]] LPVOID lpvReserved) {
    // Skip hooking in Detours helper processes
    if (DetourIsHelperProcess()) {
        return TRUE;
    }

    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            OutputDebugStringA("[AffinityHook] DLL loaded, installing hook...");

            g_hModule = hinstDLL;

            if (!LoadFunctionReferences()) {
                return FALSE;
            }

            if (!PinDllToMemory(reinterpret_cast<LPCWSTR>(hinstDLL))) {
                return FALSE;
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

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            // These should not occur due to DisableThreadLibraryCalls, but handle gracefully
            break;

        default:
            OutputDebugStringA("[AffinityHook] DLL event with unknown fdwReason - ignoring");
            break;
    }

    return TRUE;
}