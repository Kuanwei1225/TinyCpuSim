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
    python3 "${PROJECT_ROOT}/scripts/experiment.py" "$@"
}

show_help() {
    print_banner
    echo "TinyCpuSim Unified Command Line & Menu Interface Manual"
    echo "============================================================"
    echo "Workflow Steps:"
    echo "  ./run.sh build                      # [Step 1] Build simulator (Release mode)"
    echo "  ./run.sh test                       # [Step 2] Run 156+ unit & regression tests"
    echo "  ./run.sh ubench [bpu|exec|rob|cache|all] # [Step 3] Run component microbenchmarks"
    echo "  ./run.sh sim [elf] [config]         # [Step 4] Run simulation (zero-args reads current.cfg)"
    echo "  ./run.sh gem5 [--all]               # [Step 5] Compare accuracy vs gem5 golden"
    echo ""
    echo "Configuration & Editing:"
    echo "  ./run.sh edit                       # Direct vi editing of active configs/current.cfg"
    echo "  ./run.sh show                       # Display full active microarchitecture dashboard"
    echo "  ./run.sh list                       # List all presets and snapshots in default/, save/, sweep/"
    echo "  ./run.sh config                     # Launch interactive configuration manager TUI"
    echo ""
    echo "Microarchitectural Experiments:"
    echo "  ./run.sh exp                        # Run experiment on active config (configs/current.cfg) vs baseline"
    echo "  ./run.sh exp [elf]                  # Run experiment on target ELF using active config"
    echo "  ./run.sh exp --set k=v              # Run experiment with hardware overrides (e.g. ooo=false)"
    echo "  ./run.sh sweep                      # Run batch parameter sweep on configs/sweep/"
    echo ""
    echo "Catalogs & Utilities:"
    echo "  ./run.sh knobs                      # List all tunable hardware parameters & units"
    echo "  ./run.sh elfs                       # List all built-in benchmark ELF workloads"
    echo "  ./run.sh setup                      # Install required system & Python dependencies"
    echo "  ./run.sh clean                      # Clean build artifacts"
    echo "============================================================"
}

show_menu() {
    print_banner
    echo "Please choose a step or action:"
    echo "  [C] Config:   Configure Active Simulation, Hardware Knobs, Presets, Save/Load"
    echo "  [1] Step 1:   Build Project (Release Mode)"
    echo "  [2] Step 2:   Run Full Test Suite (156+ Unit & Regression Tests)"
    echo "  [3] Step 3:   Run Component Microbenchmarks (uBench)"
    echo "  [4] Step 4:   Run CPU Simulation (reads configs/current.cfg automatically)"
    echo "  [5] Step 5:   Compare Accuracy against gem5 Golden Reference"
    echo "  [6] Exp:      Run Experiment on Active Config vs Baseline"
    echo "  [7] Sweep:    Run Batch Parameter Sweep on configs/sweep/"
    echo "  [H] Help:     View Complete Command & Usage Manual"
    echo "  [0] Exit"
    echo "============================================================"
    read -r -p "Enter choice [C, 1-7, H, 0]: " choice
    case "${choice}" in
        c|C|config) python3 "${PROJECT_ROOT}/scripts/config.py" ;;
        1) "${PROJECT_ROOT}/scripts/01_build.sh" ;;
        2) "${PROJECT_ROOT}/scripts/02_run_tests.sh" ;;
        3) "${PROJECT_ROOT}/scripts/03_run_ubench.sh" ;;
        4) "${PROJECT_ROOT}/scripts/04_run_simulation.sh" ;;
        5) "${PROJECT_ROOT}/scripts/05_compare_gem5.sh" --all ;;
        6) run_interactive_exp ;;
        7) python3 "${PROJECT_ROOT}/scripts/config.py" sweep ;;
        h|H|help) show_help ;;
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
            python3 "${PROJECT_ROOT}/scripts/experiment.py" "$@"
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
        setup|deps|install-deps)
            "${PROJECT_ROOT}/scripts/install_deps.sh" "$@"
            ;;
        clean)
            echo "Cleaning build directory..."
            rm -rf "${PROJECT_ROOT}/build"
            echo "[OK] Cleaned."
            ;;
        -h|--help|help)
            show_help
            ;;
        *)
            echo "Unknown command: ${COMMAND}"
            echo "Run './run.sh --help' for available commands."
            exit 1
            ;;
    esac
fi
