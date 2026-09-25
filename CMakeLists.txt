cmake_minimum_required(VERSION 3.15)
project(TinyArmSim
    VERSION 0.1.0
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if (MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Core Library
add_library(tinyarmsim_core INTERFACE)
target_include_directories(tinyarmsim_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Main Executable CLI
add_executable(tinyarmsim src/main.cpp)
target_link_libraries(tinyarmsim PRIVATE tinyarmsim_core)

# GoogleTest via FetchContent
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable CTest
enable_testing()

# Smoke Test
add_executable(smoke_test tests/smoke_test.cpp)
target_link_libraries(smoke_test PRIVATE tinyarmsim_core gtest_main)
include(GoogleTest)
gtest_discover_tests(smoke_test)

# State and Memory Test
add_executable(state_and_memory_test tests/state_and_memory_test.cpp)
target_link_libraries(state_and_memory_test PRIVATE tinyarmsim_core gtest_main)
gtest_discover_tests(state_and_memory_test)

# Basic Data Processing Test
add_executable(basic_data_processing_test tests/basic_data_processing_test.cpp)
target_link_libraries(basic_data_processing_test PRIVATE tinyarmsim_core gtest_main)
gtest_discover_tests(basic_data_processing_test)

# Extended Data Processing Test
add_executable(extended_data_processing_test tests/extended_data_processing_test.cpp)
target_link_libraries(extended_data_processing_test PRIVATE tinyarmsim_core gtest_main)
gtest_discover_tests(extended_data_processing_test)

# Branching and Control Flow Test
add_executable(branching_and_control_flow_test tests/branching_and_control_flow_test.cpp)
target_link_libraries(branching_and_control_flow_test PRIVATE tinyarmsim_core gtest_main)
gtest_discover_tests(branching_and_control_flow_test)

# Load/Store and Stack Test
add_executable(load_store_and_stack_test tests/load_store_and_stack_test.cpp)
target_link_libraries(load_store_and_stack_test PRIVATE tinyarmsim_core gtest_main)
gtest_discover_tests(load_store_and_stack_test)

# System Control and SVC Test
add_executable(system_control_and_svc_test tests/system_control_and_svc_test.cpp)
target_link_libraries(system_control_and_svc_test PRIVATE tinyarmsim_core gtest_main)
gtest_discover_tests(system_control_and_svc_test)

# Loader Test
add_executable(loader_test tests/loader_test.cpp)
target_link_libraries(loader_test PRIVATE tinyarmsim_core gtest_main)
gtest_discover_tests(loader_test)

# Disassembler Test
add_executable(disassembler_test tests/disassembler_test.cpp)
target_link_libraries(disassembler_test PRIVATE tinyarmsim_core gtest_main)
gtest_discover_tests(disassembler_test)

# ARM GCC Cross-Compiler Detection for Assembly Tests
find_program(ARM_NONE_EABI_GCC arm-none-eabi-gcc)
if (ARM_NONE_EABI_GCC)
    message(STATUS "Found arm-none-eabi-gcc: ${ARM_NONE_EABI_GCC}")
    file(GLOB ASM_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tests/asm/*.s")
    foreach(ASM_SRC ${ASM_SOURCES})
        get_filename_component(ASM_NAME ${ASM_SRC} NAME_WE)
        set(ELF_OUT "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/${ASM_NAME}.elf")
        add_custom_command(
            OUTPUT ${ELF_OUT}
            COMMAND ${ARM_NONE_EABI_GCC} -mcpu=cortex-m3 -mthumb -nostdlib -T "${CMAKE_CURRENT_SOURCE_DIR}/tests/asm/linker.ld" "${ASM_SRC}" -o "${ELF_OUT}"
            DEPENDS "${ASM_SRC}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/asm/linker.ld"
            COMMENT "Assembling ${ASM_NAME}.s -> ${ASM_NAME}.elf"
        )
        list(APPEND ASM_ELFS ${ELF_OUT})
    endforeach()
    add_custom_target(build_asm_fixtures ALL DEPENDS ${ASM_ELFS})
endif()

# Assembly Self Test (Integration Test)
add_executable(assembly_self_test tests/assembly_self_test.cpp)
target_link_libraries(assembly_self_test PRIVATE tinyarmsim_core gtest_main)
if (TARGET build_asm_fixtures)
    add_dependencies(assembly_self_test build_asm_fixtures)
endif()
gtest_discover_tests(assembly_self_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
