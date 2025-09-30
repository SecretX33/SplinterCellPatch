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

This project uses CMake with C++26 standard. The current setup generates a static library but needs to be changed to a shared library (DLL) for the hook implementation.

**Build commands:**
```bash
# Configure the build (from project root)
cmake -B build -G Ninja

# Build the project
cmake --build build

# For Visual Studio instead of Ninja
cmake -B build -G "Visual Studio 17 2022" -A x64  # or Win32 for x86
cmake --build build --config Release
```

**Important:** The CMakeLists.txt currently uses `add_library(SplinterCellPatch STATIC ...)` which needs to be changed to `SHARED` to produce a DLL for injection.

## Technical Requirements

### Core Functionality
1. **DLL Hook**: Intercept SetProcessAffinityMask calls
2. **Parameter Override**: Change affinity mask from 0x1 to 0xFFFFFFFFFFFFFFFF
3. **Transparency**: Application should work normally with modified affinity
4. **Debug Logging**: Confirm hook operation via DebugView

### Architecture

**Current state:** Skeleton CMake project with placeholder library.cpp/library.h files that need to be replaced with the actual hook implementation.

**Target architecture:**
- Single DLL file with minimal dependencies
- Microsoft Detours library for API hooking (needs to be added to the project)
- DllMain with DLL_PROCESS_ATTACH: Install hook using DetourTransactionBegin/DetourAttach/DetourTransactionCommit
- DllMain with DLL_PROCESS_DETACH: Remove hook using DetourTransactionBegin/DetourDetach/DetourTransactionCommit
- Thread-safe implementation (Detours handles this)

**Key implementation points:**
- Function pointer typedef for the original SetProcessAffinityMask
- Hooked function that modifies the affinity mask parameter from 0x1 to 0xFFFFFFFFFFFFFFFF
- OutputDebugStringA logging with "[AffinityHook]" prefix
- Check DetourIsHelperProcess() in DllMain to skip hooking in Detours helper processes

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
1. Compile successfully (no errors or warnings)
2. Inject DLL into target application (user handles injection separately)
3. Use DebugView to verify hook installation and interception logs
4. Monitor Task Manager: CPU usage should spread across all cores (not just core 0)
5. Ensure application remains stable and functional

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
