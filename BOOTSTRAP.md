You are a C++ Windows systems programmer tasked with creating a DLL that hooks the Windows API function SetProcessAffinityMask using Microsoft Detours library.

# PROJECT CONTEXT

The user owns a legacy binary application where the original developer is deceased and source code is unavailable. This application hardcodes CPU affinity settings that restrict the process to a single CPU core (affinity mask 0x1), causing severe performance degradation on modern multi-core systems. The user has confirmed through debugger analysis that the application calls SetProcessAffinityMask(hProcess, 0x1), limiting execution to core 0 only.

# OBJECTIVE

Create a production-ready DLL using Microsoft Detours that:
1. Intercepts all calls to SetProcessAffinityMask
2. Overrides the affinity mask parameter to 0xFFFFFFFFFFFFFFFF (all cores available)
3. Allows the original function to execute with the modified parameter
4. Includes debug logging to confirm hook installation and interception
5. Handles both 32-bit and 64-bit architectures
6. Is safe, reliable, and follows Windows development best practices

**IMPORTANT: Keep this project as SIMPLE as possible**
- This is NOT enterprise code - implement straightforward, minimal solutions
- Avoid over-engineering and unnecessary abstractions
- One DLL file with direct hooking logic is sufficient
- AI agents must prioritize simplicity at all times

# TECHNICAL REQUIREMENTS

## Hook Behavior
- When the application calls SetProcessAffinityMask(handle, 0x1), the hook should modify it to SetProcessAffinityMask(handle, 0xFFFFFFFFFFFFFFFF)
- The original function must still execute (with modified parameters) to maintain application stability
- Return the actual return value from the real SetProcessAffinityMask function
- The hook should be transparent to the application

## Detours Integration
- Use Microsoft Detours library for API hooking (check if it's already installed/available, and if not, add it to the project's files)
- Implement proper hook installation in DLL_PROCESS_ATTACH
- Implement proper hook removal in DLL_PROCESS_DETACH
- Use DetourTransactionBegin/Commit pattern for atomic hook installation
- Handle DetourIsHelperProcess() check for Detours compatibility

## Debug Logging
- Use OutputDebugStringA for logging (viewable in DebugView)
- Log when the DLL is loaded and hook is installed
- Log each interception with original and modified affinity masks
- Log hook removal on DLL unload
- Format: "[AffinityHook] message" for easy filtering

## Code Quality
- Clean, well-commented code explaining each section
- Proper error handling for Detours operations
- Thread-safe implementation
- No memory leaks
- Compatible with both Debug and Release builds

# DELIVERABLES

Provide complete, copy-paste ready C++ code including:

1. **Full DLL source code** (single .cpp file with all necessary includes)
2. **Compilation instructions** for Visual Studio or command-line build
3. **Architecture considerations** (x86 vs x64 targeting)
4. **Testing instructions** for verifying the hook works correctly

# CODE STRUCTURE

The DLL should follow this structure:

```cpp
// Required includes
#include <windows.h>
#include <detours.h>
#include <stdio.h>

// Function pointer typedef for original function
// Hooked function implementation
// DLL entry point with hook installation/removal
// Helper functions if needed
```

# IMPORTANT CONSTRAINTS

- The user will handle DLL injection separately - do not include injection code
- Focus only on the hook DLL implementation
- Check if Microsoft Detours is available and properly linked
- The DLL must work with the exact signature: BOOL WINAPI SetProcessAffinityMask(HANDLE hProcess, DWORD_PTR dwProcessAffinityMask)
- Maintain compatibility with Windows 10 and Windows 11

# SUCCESS CRITERIA

The solution is successful when:
1. The DLL compiles without errors or warnings
2. When injected, the application uses all CPU cores instead of just core 0
3. Debug logs confirm interception is occurring
4. The application remains stable and functional
5. Task Manager shows CPU usage distributed across all cores

# EXPECTED OUTPUT FORMAT

Provide your response in this format:

## Complete DLL Source Code
[Full .cpp file with all code]

## Compilation Instructions
[Step-by-step build instructions]

## Testing & Verification
[How to verify the hook is working]

## Technical Notes
[Any important implementation details, platform-specific considerations, or gotchas]

Generate the complete, production-ready code now.
```

---

## Implementation Notes

**Key techniques used:**
- **Comprehensive context provision**: Includes the full problem background, user's investigation findings, and technical constraints
- **Clear objective definition**: Specifies exactly what the hook should do (intercept and modify, not block)
- **Technical specifications**: Details the exact behavior, function signatures, and Detours patterns
- **Success criteria**: Provides measurable outcomes for verification
- **Structured output format**: Ensures the AI delivers organized, usable code

**Why this approach:**
- Provides all context needed for the AI to understand this is legitimate software modification
- Includes debugger trace findings to show the exact problem (affinity mask 0x1)
- Specifies Microsoft Detours to ensure a reliable, industry-standard solution
- Emphasizes safety and stability (calling original function, not blocking it)

## Testing & Evaluation

**Suggested test cases:**
1. Verify the code compiles in both Debug and Release configurations
2. Check that all Detours API calls are used correctly
3. Ensure the hook doesn't crash the target application
4. Validate that CPU usage spreads across all cores after injection
5. Test with DebugView running to confirm logging works

**Edge cases to consider:**
- Multiple threads calling SetProcessAffinityMask simultaneously
- Repeated attach/detach of the DLL
- Different Windows versions (10, 11)
- Both x86 and x64 target applications