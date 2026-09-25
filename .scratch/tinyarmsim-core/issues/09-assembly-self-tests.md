# 09: Automated Assembly Self-Test Framework & CI Fixtures

**What to build:** The automated end-to-end self-test framework integrating cross-compiled ARM assembly tests: CMake custom targets invoking `arm-none-eabi-gcc` when available, checked-in precompiled ELF fixtures for regression testing without toolchain dependencies, and a test suite verifying full assembly algorithms (fibonacci, memory copy, sorting).

**Blocked by:** 07-system-control-and-svc, 08-dual-mode-binary-loader

**Status:** resolved

- [x] CMake module checks for `arm-none-eabi-gcc` and creates targets to compile `.s` files in `tests/asm/` into `.elf` binaries
- [x] Pre-compiled `.elf` test fixtures committed under `tests/fixtures/`
- [x] Assembly tests cover arithmetic operations, function calls/stack frames, loops, and termination via `SVC #0`
- [x] Integration tests load ELF binaries and assert `IsaInterpreter::run()` halts with exit code 0 in register R0
- [x] CTest executes the entire regression suite cleanly
