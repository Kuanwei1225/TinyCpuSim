# 01: Project Scaffold, CMake Build System, and Test Harness

**What to build:** A clean C++17 CMake project scaffold configured with GoogleTest, standard compilation warnings, and an initial test suite verifying the build harness works end-to-end.

**Blocked by:** None (can start immediately).

**Status:** resolved

- [x] CMakeLists.txt configures C++17 with `-Wall -Wextra -Wpedantic` (or MSVC equivalent)
- [x] GoogleTest is integrated cleanly (via FetchContent or system package)
- [x] `include/` and `src/` project directory layout created with basic namespace definitions
- [x] A baseline smoke test executes and passes under `ctest`
