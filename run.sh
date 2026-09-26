#!/usr/bin/env bash
# TinyCpuSim Unified Top-Level Interactive Launcher & Workflow Manager
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIGS_DIR="${PROJECT_ROOT}/configs"
FIXTURES_DIR="${PROJECT_ROOT}/tests/fixtures"

print_banner() {
    echo "============================================================"
    echo "       TinyCpuSim - ARM Out-of-Order CPU Simulator          "
    echo "============================================================"
}

run_interactive_exp() {
    echo ""
    echo "============================================================"
    echo "           Microarchitectural Experiment Wizard             "
    echo "============================================================"
    echo "Select target benchmark workload ELF:"
    echo "  [1] test_fibonacci.elf (Recursive call stack, branching, ALU)"
    echo "  [2] test_sort.elf      (Bubble sort, conditional branches, LSU)"
    echo "  [3] test_stress.elf    (10k loop, maximum issue saturation)"
    echo "  [4] test_mem_stride.elf(Strided memory accesses, cache misses)"
    echo "  [5] test_store_forward.elf (Store-to-load forwarding bypass)"
    echo "  [6] test_raw_hazard.elf(Chained RAW data dependencies)"
    echo "  [7] test_branch_pred.elf (Mixed/alternating branch patterns)"
    echo "============================================================"
    read -r -p "Enter ELF choice [1-7, default: 1]: " elf_choice
    local target_elf="test_fibonacci.elf"
    case "${elf_choice}" in
        2) target_elf="test_sort.elf" ;;
        3) target_elf="test_stress.elf" ;;
        4) target_elf="test_mem_stride.elf" ;;
        5) target_elf="test_store_forward.elf" ;;
        6) target_elf="test_raw_hazard.elf" ;;
        7) target_elf="test_branch_pred.elf" ;;
        *) target_elf="test_fibonacci.elf" ;;
    esac

    echo ""
    echo "Enter parameter modification (e.g. rob=128, ooo=false, width=8, bp_enabled=false, l1d_size=64KB):"
    read -r -p "Override [default: rob=128]: " param_override
    [ -z "${param_override}" ] && param_override="rob=128"

    echo ""
    echo "Select baseline config (e.g. default, multicore, or snapshot in configs/save/):"
    read -r -p "Baseline [default: default]: " base_choice
    [ -z "${base_choice}" ] && base_choice="default"

    python3 "${PROJECT_ROOT}/scripts/experiment.py" --elf "${target_elf}" --config "${base_choice}" --set "${param_override}"
}

show_menu() {
    print_banner
    echo "Please choose a step or action:"
    echo "  [C] Config: Configure Active Simulation, Hardware Knobs, Presets, Save/Load"
    echo "  [1] Step 1: Build Project (Release Mode)"
    echo "  [2] Step 2: Run Full Test Suite (156+ Unit & Regression Tests)"
    echo "  [3] Step 3: Run Component Microbenchmarks (uBench)"
    echo "  [4] Step 4: Run CPU Simulation (reads configs/current.cfg automatically)"
    echo "  [5] Step 5: Compare Accuracy against gem5 Golden Reference"
    echo "  [6] Exp:    Run Parameter Experiment vs Baseline on Target ELF"
    echo "  [7] Sweep:  Run Batch Parameter Sweep on configs/sweep/"
    echo "  [0] Exit"
    echo "============================================================"
    read -r -p "Enter choice [C, 1-7, 0]: " choice
    case "${choice}" in
        c|C|config) python3 "${PROJECT_ROOT}/scripts/config.py" ;;
        1) "${PROJECT_ROOT}/scripts/01_build.sh" ;;
        2) "${PROJECT_ROOT}/scripts/02_run_tests.sh" ;;
        3) "${PROJECT_ROOT}/scripts/03_run_ubench.sh" all ;;
        4) "${PROJECT_ROOT}/scripts/04_run_simulation.sh" ;;
        5) "${PROJECT_ROOT}/scripts/05_compare_gem5.sh" --all ;;
        6) run_interactive_exp ;;
        7) python3 "${PROJECT_ROOT}/scripts/config.py" sweep ;;
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
        c|cfg|config)
            python3 "${PROJECT_ROOT}/scripts/config.py" "$@"
            ;;
        edit|vi)
            python3 "${PROJECT_ROOT}/scripts/config.py" edit
            ;;
        show|view)
            python3 "${PROJECT_ROOT}/scripts/config.py" show
            ;;
        list|ls)
            python3 "${PROJECT_ROOT}/scripts/config.py" list
            ;;
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
            if [ $# -eq 0 ]; then
                run_interactive_exp
            else
                python3 "${PROJECT_ROOT}/scripts/experiment.py" "$@"
            fi
            ;;
        7|sweep)
            python3 "${PROJECT_ROOT}/scripts/config.py" sweep "$@"
            ;;
        knobs|params|list-params)
            python3 "${PROJECT_ROOT}/scripts/experiment.py" --list-params
            ;;
        elfs|list-elfs)
            python3 "${PROJECT_ROOT}/scripts/experiment.py" --list-elfs
            ;;
        clean)
            echo "Cleaning build directory..."
            rm -rf "${PROJECT_ROOT}/build"
            echo "[OK] Cleaned."
            ;;
        -h|--help|help)
            print_banner
            echo "Direct CLI Commands:"
            echo "  ./run.sh                            # Launch Interactive Menu"
            echo "  ./run.sh edit                       # Open active config in vi directly"
            echo "  ./run.sh show                       # Display full active microarchitecture dashboard"
            echo "  ./run.sh list                       # List all presets and snapshots in default/, save/, sweep/"
            echo "  ./run.sh config [menu|edit|show...] # Launch configuration manager"
            echo "  ./run.sh build                      # [Step 1] Build simulator and tests"
            echo "  ./run.sh test                       # [Step 2] Run 156+ unit tests"
            echo "  ./run.sh ubench [component|all]     # [Step 3] Run microbenchmarks (bpu, exec, rob, cache, all)"
            echo "  ./run.sh sim [elf] [config]         # [Step 4] Run simulation (zero-args: reads current.cfg)"
            echo "  ./run.sh gem5 [--all]               # [Step 5] Compare against gem5 golden"
            echo "  ./run.sh exp                        # Run Interactive Parameter Experiment Wizard"
            echo "  ./run.sh exp --elf <elf> --set k=v  # Run CLI parameter experiment vs baseline"
            echo "  ./run.sh sweep                      # Run batch parameter sweep on configs/sweep/"
            echo "  ./run.sh knobs                      # List all tunable hardware parameters catalog"
            echo "  ./run.sh elfs                       # List all available test ELF workloads"
            echo "  ./run.sh clean                      # Clean build directory"
            echo "============================================================"
            ;;
        *)
            echo "Unknown command: ${COMMAND}"
            echo "Run './run.sh --help' for available commands."
            exit 1
            ;;
    esac
fi
