# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a Windows DLL hook project that patches the SetProcessAffinityMask Windows API call in legacy applications to enable multi-core CPU usage. The target application hardcodes CPU affinity to a single core (0x1), causing performance issues on modern multi-core systems.

**Primary Goal: SIMPLICITY ABOVE ALL**
- This is NOT enterprise code
- Implement the simplest solution that works
- Avoid over-engineering
- Keep the codebase minimal and maintainable
- One DLL file with straightforward hooking logic

## Development Philosophy

### Core Principles
1. **Conservative Development**: Make surgical, precise changes. Don't refactor unless necessary.
2. **Simplicity First**: Always choose the simpler solution. Avoid abstractions unless they provide clear value.
3. **Respect Existing Patterns**: Follow established code patterns in the codebase.
4. **Collaborative Approach**: Work with the developer as a pair programmer.

### Critical Rules
- **NEVER** create files unless absolutely necessary
- **ALWAYS** prefer editing existing files over creating new ones
- **NEVER** proactively create documentation files unless explicitly requested
- **NO** unnecessary abstractions or design patterns
- **NO** premature optimization
- Keep it simple, stupid (KISS principle)

## C++ and Windows Development Guidelines

### Code Style
- Use modern C++ (C++17 or later) features when appropriate
- Follow Windows API naming conventions
- Use RAII for resource management
- Prefer stack allocation over heap when possible
- Use `nullptr` instead of `NULL`
- Use `auto` for complex type declarations when it improves readability

### Naming Conventions
- **Functions**: PascalCase for Windows API style (`SetProcessAffinityMask`)
- **Variables**: camelCase (`dwProcessAffinityMask`)
- **Constants**: UPPER_SNAKE_CASE (`MAX_BUFFER_SIZE`)
- **Types**: PascalCase (`DWORD_PTR`)
- Use descriptive names that explain purpose

### Function Design
- Keep functions small and focused (single responsibility)
- Minimize function parameters (ideally ≤3)
- Use const correctness
- Handle errors gracefully with proper return codes
- Add comments for complex logic, not obvious code

### Memory Management
- Use RAII for all resources (handles, memory, etc.)
- Avoid manual `new`/`delete` - use smart pointers if needed
- Clean up in DLL_PROCESS_DETACH
- No memory leaks - verify with tools if necessary

### Error Handling
- Check return values from Windows API calls
- Log errors using OutputDebugString
- Fail gracefully - don't crash the host application
- Use RAII to ensure cleanup happens even on error paths

### Logging
- Use OutputDebugString for debug logging
- Format: `[AffinityHook] message`
- Log important events: hook install, interception, unhook
- Keep logs concise and actionable

## Build System

This project uses CMake with C++26 standard and builds as a shared library (DLL).

### Project Structure
```
SplinterCellPatch/
├── src/                  # Source files
│   ├── library.cpp       # Main hook implementation
│   └── library.h         # Header file
├── lib/                  # Precompiled Detours libraries
│   ├── detours_x64.lib   # 64-bit Detours library
│   └── detours_x86.lib   # 32-bit Detours library
├── include/              # Detours headers
│   ├── detours_x64.h     # 64-bit Detours header
│   └── detours_x86.h     # 32-bit Detours header
└── CMakeLists.txt        # Build configuration
```

### Building with CLion (Recommended)
1. **File → Open** → Select project folder
2. CLion auto-detects CMakeLists.txt and configures
3. Select build configuration (Debug/Release) from top-right dropdown
4. Click **Build** (hammer icon) or press **Ctrl+F9**
5. Output DLL: `cmake-build-release/SplinterCellPatch.dll`

### Building with Visual Studio
1. **File → Open → Folder** → Select project folder
2. VS auto-detects CMakeLists.txt
3. Select configuration (x64-Debug/x64-Release) from toolbar
4. **Build → Build All** or press **Ctrl+Shift+B**
5. Output DLL: `out/build/x64-Release/SplinterCellPatch.dll`

### Building with Command Line
```bash
# Using Developer Command Prompt for VS 2022
cd "D:\Local Disk\Users\User\Documents\TrashProjects\SplinterCellPatch"

# For x64 build
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release

# For x86 build
cmake -B build -G "Visual Studio 17 2022" -A Win32
cmake --build build --config Release

# Output: build/Release/SplinterCellPatch.dll
```

### Detours Integration
The project uses precompiled Detours libraries (v3.0 Build_343) located in `lib/` with headers in `include/`. CMake automatically selects the correct library and header based on target architecture (x64 vs x86).

## Technical Requirements

### Core Functionality
1. **DLL Hook**: Intercept SetProcessAffinityMask calls
2. **Parameter Override**: Change affinity mask from 0x1 to 0xFFFFFFFFFFFFFFFF
3. **Transparency**: Application should work normally with modified affinity
4. **Debug Logging**: Confirm hook operation via DebugView

### Architecture

**Current state:** Fully implemented hook DLL ready for injection.

**Implementation:**
- Single DLL file with minimal dependencies
- Microsoft Detours library v3.0 for API hooking (precompiled .lib files in `lib/`)
- `src/library.cpp:` Main implementation with DllMain entry point
  - **DLL_PROCESS_ATTACH**: Install hook using DetourTransactionBegin/DetourAttach/DetourTransactionCommit
  - **DLL_PROCESS_DETACH**: Remove hook using DetourTransactionBegin/DetourDetach/DetourTransactionCommit
  - Checks `DetourIsHelperProcess()` to skip hooking in Detours helper processes
- Thread-safe implementation (Detours handles this automatically)

**Key implementation details:**
- Function pointer `TrueSetProcessAffinityMask` stores original function address
- `HookedSetProcessAffinityMask()` intercepts calls and modifies affinity mask from 0x1 to 0xFFFFFFFFFFFFFFFF
- All events logged via `OutputDebugStringA()` with "[AffinityHook]" prefix
- Architecture-specific header includes via preprocessor directives (`_M_X64` for x64, else x86)

### Compatibility
- Windows 10 and 11
- Both x86 and x64 architectures (build separate DLLs for each)
- Debug and Release builds

## Development Workflow

### When Making Changes
1. **Understand First**: Read existing code before modifying
2. **Minimal Changes**: Change only what's necessary
3. **Test Thoroughly**: Verify hook works after changes
4. **No Refactoring**: Unless explicitly requested or absolutely necessary

### Code Review Checklist
- [ ] Does it solve the problem with the simplest approach?
- [ ] Are all Windows API calls checked for errors?
- [ ] Is memory properly managed (no leaks)?
- [ ] Are hooks properly installed and removed?
- [ ] Is logging added for debugging?
- [ ] Does it compile without warnings?
- [ ] Is it thread-safe?

### Testing Verification
1. **Build**: Compile successfully (no errors or warnings)
2. **Prepare monitoring**: Download and run [DebugView](https://learn.microsoft.com/en-us/sysinternals/downloads/debugview)
   - Enable **Capture → Capture Global Win32**
   - Optional: Filter for "AffinityHook" to see only relevant logs
3. **Inject**: Inject DLL into target application (user handles injection separately)
4. **Verify logs**: Check DebugView for:
   - `[AffinityHook] DLL loaded, installing hook...`
   - `[AffinityHook] Hook installed successfully`
   - `[AffinityHook] Intercepted SetProcessAffinityMask call - Original mask: 0x1`
   - `[AffinityHook] Modifying mask to: 0xFFFFFFFFFFFFFFFF (all cores)`
5. **Verify performance**: Monitor Task Manager - CPU usage should spread across all cores (not just core 0)
6. **Verify stability**: Ensure application remains stable and functional

## Common Pitfalls to Avoid

### Don't
- Over-engineer solutions
- Add unnecessary abstractions
- Create multiple files when one suffices
- Implement features not explicitly requested
- Refactor working code without reason
- Add complex error handling for simple cases

### Do
- Keep it simple
- Use proven patterns (Detours standard usage)
- Log important events
- Handle errors gracefully
- Write clear, readable code

---

**Remember**: This project values simplicity and directness. When in doubt, choose the simpler solution. This is a focused, single-purpose DLL that should remain minimal and maintainable. See BOOTSTRAP.md for detailed requirements.
