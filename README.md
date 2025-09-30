# SplinterCellPatch - CPU Affinity Hook DLL

A Windows DLL that hooks `SetProcessAffinityMask` to enable multi-core CPU usage in legacy applications that hardcode single-core affinity.

## Overview

This DLL uses Microsoft Detours to intercept calls to `SetProcessAffinityMask` and override the affinity mask from `0x1` (single core) to `0xFFFFFFFFFFFFFFFF` (all cores available), allowing legacy applications to utilize modern multi-core processors.

## Project Structure

```
SplinterCellPatch/
├── src/
│   ├── library.cpp       # Main hook implementation
│   └── library.h         # Header file
├── lib/
│   ├── detours_x64.lib   # 64-bit Detours library
│   └── detours_x86.lib   # 32-bit Detours library
├── include/
│   ├── detours_x64.h     # 64-bit Detours header
│   └── detours_x86.h     # 32-bit Detours header
├── CMakeLists.txt        # Build configuration
├── CLAUDE.md             # AI agent guidelines
├── BOOTSTRAP.md          # Implementation specifications
└── README.md             # This file
```

## Building the Project

### Prerequisites

- **Visual Studio 2022** (or compatible C++ compiler)
- **CMake 4.0+**
- **CLion** (optional, recommended)

### Option 1: Build with CLion (Recommended)

1. Open CLion
2. **File → Open** → Select the project folder
3. Wait for CMake to configure (status bar shows "CMake project loaded")
4. Select build configuration from top-right dropdown:
   - **Release** for production use
   - **Debug** for development
5. Click the **Build** button (hammer icon) or press **Ctrl+F9**
6. Find output: `cmake-build-release/SplinterCellPatch.dll`

### Option 2: Build with Visual Studio

1. Open Visual Studio 2022
2. **File → Open → Folder** → Select the project folder
3. Visual Studio auto-detects `CMakeLists.txt` and configures
4. Select configuration from toolbar:
   - **x64-Release** for 64-bit production
   - **x64-Debug** for 64-bit development
   - **x86-Release** for 32-bit production
   - **x86-Debug** for 32-bit development
5. **Build → Build All** or press **Ctrl+Shift+B**
6. Find output: `out/build/x64-Release/SplinterCellPatch.dll`

### Option 3: Build via Command Line

Open **Developer Command Prompt for VS 2022**:

```cmd
# Navigate to project directory
cd "D:\Local Disk\Users\User\Documents\TrashProjects\SplinterCellPatch"

# Configure for 64-bit
cmake -B build -G "Visual Studio 17 2022" -A x64

# Build Release configuration
cmake --build build --config Release

# Output: build/Release/SplinterCellPatch.dll
```

For **32-bit build**, replace `-A x64` with `-A Win32`.

## Using the DLL

### 1. Build the DLL

Follow one of the build methods above to create `SplinterCellPatch.dll`.

### 2. Inject into Target Application

Use a DLL injector of your choice to inject the DLL into the target application. Common options:
- **Process Hacker** (right-click process → Miscellaneous → Inject DLL)
- **Extreme Injector**
- **Custom injector**

**Note:** Make sure to use the correct architecture (x64 DLL for x64 apps, x86 DLL for x86 apps).

### 3. Verify Operation

The hook is working when you observe:
- CPU usage distributed across all cores in Task Manager
- Debug logs confirming interception (see Debugging section below)
- Application running normally with improved performance

## Debugging

### Viewing Debug Logs

The DLL outputs debug information via `OutputDebugStringA()`. To view these logs:

#### Method 1: DebugView (Recommended)

1. **Download DebugView** from [Sysinternals](https://learn.microsoft.com/en-us/sysinternals/downloads/debugview)
2. Run **Dbgview.exe** (no installation needed)
3. Enable **Capture → Capture Global Win32** (should be enabled by default)
4. *Optional:* **Edit → Filter/Highlight** → Add filter `AffinityHook` to show only relevant messages
5. Inject the DLL into target application
6. Watch for log messages:

```
[AffinityHook] DLL loaded, installing hook...
[AffinityHook] Hook installed successfully
[AffinityHook] Intercepted SetProcessAffinityMask call - Original mask: 0x1
[AffinityHook] Modifying mask to: 0xFFFFFFFFFFFFFFFF (all cores)
```

#### Method 2: Visual Studio Debugger

1. **Debug → Attach to Process**
2. Select the target application process
3. View debug output in **View → Output** window
4. Look for `[AffinityHook]` messages

#### Method 3: WinDbg

If you have Windows SDK/WinDbg installed:
1. Attach WinDbg to the target process
2. Debug output appears in command window

### Common Debug Scenarios

**Hook not installing:**
```
[AffinityHook] ERROR: Hook installation failed
```
- Check that target application architecture matches DLL architecture (x64/x86)
- Verify Detours library is properly linked
- Ensure no conflicts with other hooks or security software

**No interception messages:**
- Confirm the application is actually calling `SetProcessAffinityMask`
- Check that DLL was successfully injected
- Verify DebugView is capturing global Win32 messages

**Hook removal errors:**
```
[AffinityHook] ERROR: Hook removal failed
```
- Usually harmless if application is exiting
- May indicate hook was already removed or modified by another component

### Debugging Build Issues

**CMake configuration fails:**
- Ensure CMake 4.0+ is installed
- Check that Visual Studio 2022 C++ tools are installed
- Verify `CMakeLists.txt` is in project root

**Link errors (cannot find detours.lib):**
- Check that `lib/detours_x64.lib` and `lib/detours_x86.lib` exist
- Verify `include/detours_x64.h` and `include/detours_x86.h` exist
- Ensure you're building for the correct architecture

**Header not found errors:**
- Verify files exist in `include/` directory
- Check CMakeLists.txt includes the include directory
- Ensure architecture-specific headers match build target

## Testing

### Manual Testing Checklist

- [ ] DLL compiles without errors or warnings
- [ ] DLL successfully injects into target application
- [ ] DebugView shows hook installation message
- [ ] DebugView shows interception messages when app starts
- [ ] Task Manager shows CPU usage across multiple cores
- [ ] Application remains stable and functional
- [ ] Application performance improves (verify via testing/benchmarks)
- [ ] DLL unloads cleanly when application exits

### Expected Behavior

**Before hook:**
- CPU usage shows on Core 0 only (or minimal usage on other cores)
- Task Manager shows ~12.5% CPU on 8-core system (100% of 1 core)

**After hook:**
- CPU usage distributed across all available cores
- Task Manager shows variable usage across all cores
- Overall performance improvement in CPU-bound scenarios

## Troubleshooting

### Issue: DLL won't inject

**Solution:**
- Verify target application architecture matches DLL (x64 vs x86)
- Run injector with administrator privileges
- Check antivirus/security software isn't blocking injection

### Issue: No performance improvement

**Solution:**
- Verify hook is actually intercepting (check DebugView logs)
- Confirm application is CPU-bound (not I/O or memory bound)
- Check that application actually calls SetProcessAffinityMask on startup
- Use a profiler to verify multi-core usage

### Issue: Application crashes after injection

**Solution:**
- Check DebugView for error messages
- Try Debug build of DLL for more detailed logging
- Verify no conflicts with other hooks/overlays
- Check Windows Event Viewer for crash details

### Issue: "Access Denied" when injecting

**Solution:**
- Run injector as Administrator
- Check if target application has anti-cheat/DRM protection
- Disable User Account Control (UAC) temporarily for testing

## Technical Details

### How It Works

1. **DLL Injection:** User injects DLL into target process
2. **DllMain Called:** Windows calls `DllMain` with `DLL_PROCESS_ATTACH`
3. **Hook Installation:** Detours intercepts `SetProcessAffinityMask` in kernel32.dll
4. **Interception:** When app calls `SetProcessAffinityMask(handle, 0x1)`:
   - Control redirects to `HookedSetProcessAffinityMask()`
   - Hook logs original mask (0x1)
   - Hook modifies mask to 0xFFFFFFFFFFFFFFFF (all cores)
   - Hook calls original function with modified mask
   - Returns result to application
5. **Transparency:** Application receives success result, unaware of modification
6. **Cleanup:** On `DLL_PROCESS_DETACH`, hook is properly removed

### Architecture Support

- **x64 (64-bit):** Uses `detours_x64.lib` and `detours_x64.h`
- **x86 (32-bit):** Uses `detours_x86.lib` and `detours_x86.h`

CMake automatically selects the correct library based on target architecture.

### Compatibility

- **Operating Systems:** Windows 10, Windows 11
- **Architectures:** x86 (32-bit), x64 (64-bit)
- **Build Configurations:** Debug, Release
- **Compilers:** MSVC 2022 (C++26 standard)

## License

See project license file for details.

## Credits

- **Microsoft Detours v3.0** - API hooking library
- Built with CMake and modern C++26
