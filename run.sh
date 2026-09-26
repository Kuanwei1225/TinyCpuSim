#!/usr/bin/env bash
# TinyCpuSim Unified Top-Level Interactive Launcher & Workflow Manager
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

print_banner() {
    echo "============================================================"
    echo "       TinyCpuSim - ARM Out-of-Order CPU Simulator          "
    echo "============================================================"
}

show_menu() {
    print_banner
    echo "Please choose a step or action:"
    echo "  [1] Step 1: Build Project (Release Mode)"
    echo "  [2] Step 2: Run Full Test Suite (156+ Unit & Regression Tests)"
    echo "  [3] Step 3: Run Component Microbenchmarks (uBench)"
    echo "  [4] Step 4: Run Full CPU Simulation on Fibonacci ELF"
    echo "  [5] Step 5: Compare Accuracy against gem5 Golden Reference"
    echo "  [6] Exp:    Run Hardware Parameter Experiment on Target ELF"
    echo "  [7] Sweep:  Run Automated Parameter Sweep (ROB, Width, L1)"
    echo "  [8] Knobs:  List All Tunable Microarchitecture Parameters"
    echo "  [0] Exit"
    echo "============================================================"
    read -r -p "Enter choice [1-8, 0]: " choice
    case "${choice}" in
        1) "${PROJECT_ROOT}/scripts/01_build.sh" ;;
        2) "${PROJECT_ROOT}/scripts/02_run_tests.sh" ;;
        3) "${PROJECT_ROOT}/scripts/03_run_ubench.sh" all ;;
        4) "${PROJECT_ROOT}/scripts/04_run_simulation.sh" "${PROJECT_ROOT}/tests/fixtures/test_fibonacci.elf" --all-perf ;;
        5) "${PROJECT_ROOT}/scripts/05_compare_gem5.sh" --all ;;
        6) python3 "${PROJECT_ROOT}/scripts/experiment.py" --elf test_fibonacci.elf --set rob=128 ;;
        7) python3 "${PROJECT_ROOT}/scripts/sweep_parameters.py" --elf tests/fixtures/test_fibonacci.elf --param rob_size ;;
        8) python3 "${PROJECT_ROOT}/scripts/experiment.py" --list-params ;;
        0|q|Q) echo "Goodbye!"; exit 0 ;;
        *) echo "Invalid option."; exit 1 ;;
    esac
}

if [ $# -eq 0 ]; then
    show_menu
else
    COMMAND="$1"
    shift || true
    case "${COMMAND}" in
        1|build)
            "${PROJECT_ROOT}/scripts/01_build.sh" "$@"
            ;;
        2|test|tests)
            "${PROJECT_ROOT}/scripts/02_run_tests.sh" "$@"
            ;;
        3|ubench)
            "${PROJECT_ROOT}/scripts/03_run_ubench.sh" "$@"
            ;;
        4|sim|simulate)
            "${PROJECT_ROOT}/scripts/04_run_simulation.sh" "$@"
            ;;
        5|gem5)
            "${PROJECT_ROOT}/scripts/05_compare_gem5.sh" "$@"
            ;;
        6|exp|experiment)
            python3 "${PROJECT_ROOT}/scripts/experiment.py" "$@"
            ;;
        7|sweep)
            python3 "${PROJECT_ROOT}/scripts/sweep_parameters.py" "$@"
            ;;
        8|knobs|params|list-params)
            python3 "${PROJECT_ROOT}/scripts/experiment.py" --list-params
            ;;
        elfs|list-elfs)
            python3 "${PROJECT_ROOT}/scripts/experiment.py" --list-elfs
            ;;
        -h|--help|help)
            print_banner
            echo "Direct Command Usage:"
            echo "  ./run.sh build                      # [Step 1] Build simulator and tests"
            echo "  ./run.sh test                       # [Step 2] Run 156+ unit tests"
            echo "  ./run.sh ubench [component]         # [Step 3] Run microbenchmarks (bpu, exec, rob, cache, all)"
            echo "  ./run.sh sim <elf> [config]         # [Step 4] Run full-system simulation"
            echo "  ./run.sh gem5 [--all]               # [Step 5] Compare against gem5 golden"
            echo "  ./run.sh exp --elf <name> --set k=v # Run single/multi ELF experiment vs baseline"
            echo "  ./run.sh sweep [options]            # Run hardware parameter sweep"
            echo "  ./run.sh knobs                      # List all tunable hardware parameters"
            echo "  ./run.sh elfs                       # List all available test ELF workloads"
            echo "  ./run.sh                            # Launch interactive menu"
            echo "============================================================"
            ;;
        *)
            echo "Unknown command: ${COMMAND}"
            echo "Run './run.sh --help' for available commands."
            exit 1
            ;;
    esac
fi
