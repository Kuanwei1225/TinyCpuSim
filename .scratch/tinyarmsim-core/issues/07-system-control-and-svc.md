# 07: System Control, Status Registers, and SVC Exit Contract

**What to build:** System instructions (`MRS`, `MSR`, `NOP`) and software interrupt trapping (`SVC`) where `SVC #0` halts the execution loop and returns `R0` as the simulation exit code.

**Blocked by:** 06-load-store-and-stack-operations

**Status:** resolved

- [x] `MRS` reads CPSR/APSR into destination register
- [x] `MSR` writes source register into CPSR/APSR condition flags
- [x] `NOP` executes as a no-operation advancing PC
- [x] `SVC` triggers an execution halt in `IsaInterpreter`, capturing `R0` as exit status code
- [x] Tests verify status register read/write and clean termination via `SVC #0`
