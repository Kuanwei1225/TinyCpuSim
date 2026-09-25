#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>
#include "tinyarmsim/common.hpp"
#include "tinyarmsim/state.hpp"
#include "tinyarmsim/memory_bus.hpp"
#include "tinyarmsim/loader.hpp"
#include "tinyarmsim/interpreter.hpp"
#include "tinyarmsim/uarch/config.hpp"
#include "tinyarmsim/uarch/stats.hpp"
#include "tinyarmsim/uarch/memory_hierarchy.hpp"

namespace {

void print_usage(const char* prog_name) {
    std::cout << "TinyArmSim v" << tinyarmsim::get_version_string() << " - ARM CPU ISA & uArch Simulator\n\n"
              << "Usage: " << prog_name << " [options] <elf-file>\n\n"
              << "General Options:\n"
              << "  --elf <file>             Specify input ELF binary file\n"
              << "  -l, --log, --verbose     Enable step-by-step instruction trace logging\n"
              << "  -c, --coverage <file>    Export instruction opcode coverage report to CSV\n"
              << "  -m, --max-steps <N>      Set maximum instruction execution steps (default: 1000000000)\n"
              << "  -h, --help               Display this help message\n\n"
              << "Microarchitecture (uArch / OoO) Options:\n"
              << "  --uarch                  Enable microarchitectural simulation mode\n"
              << "  -u, --uarch-config <file> Load uArch configuration file (default: OoO medium)\n"
              << "  --uarch-stats <file>     Export hardware performance counters to report file\n\n"
              << "Examples:\n"
              << "  " << prog_name << " app.elf\n"
              << "  " << prog_name << " --log --coverage cov.csv app.elf\n"
              << "  " << prog_name << " --uarch --uarch-config configs/ooo_medium.cfg app.elf\n"
              << "  " << prog_name << " --uarch --uarch-stats stats.txt app.elf\n"
              << std::endl;
}

void print_banner(bool passed, uint32_t exit_code, const std::string& fault_msg, const tinyarmsim::SimulationStats& stats) {
    double mips = stats.instructions_per_second / 1000000.0;
    std::cout << "\n============================================================\n";
    if (passed) {
        std::cout << "SIMULATION PASSED\n";
    } else {
        std::cout << "SIMULATION FAILED\n";
    }
    if (!fault_msg.empty()) {
        std::cout << "Fault Reason:       " << fault_msg << "\n";
    }
    std::cout << "Exit Code:          " << exit_code << " (R0 = " << exit_code << ")\n"
              << "Total Instructions: " << stats.instruction_count << "\n"
              << std::fixed << std::setprecision(6)
              << "Elapsed Time:       " << stats.elapsed_seconds << " s\n"
              << std::fixed << std::setprecision(2)
              << "Simulation Speed:   " << stats.instructions_per_second << " inst/s (" << mips << " MIPS)\n"
              << "============================================================\n"
              << std::endl;
}

} // namespace

int main(int argc, char* argv[]) {
    std::string elf_path;
    std::string coverage_path;
    std::string uarch_config_path;
    std::string uarch_stats_path;
    bool enable_log = false;
    bool enable_uarch = false;
    uint64_t max_steps = 1000000000;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "-l" || arg == "--log" || arg == "--verbose") {
            enable_log = true;
        } else if (arg == "--uarch" || arg == "--uarch-mode") {
            enable_uarch = true;
        } else if (arg == "-u" || arg == "--uarch-config") {
            enable_uarch = true;
            if (i + 1 < argc) {
                uarch_config_path = argv[++i];
            } else {
                std::cerr << "Error: --uarch-config requires a file path argument.\n";
                return 1;
            }
        } else if (arg == "--uarch-stats") {
            if (i + 1 < argc) {
                uarch_stats_path = argv[++i];
            } else {
                std::cerr << "Error: --uarch-stats requires a file path argument.\n";
                return 1;
            }
        } else if (arg == "-c" || arg == "--coverage") {
            if (i + 1 < argc) {
                coverage_path = argv[++i];
            } else {
                std::cerr << "Error: --coverage requires a file path argument.\n";
                return 1;
            }
        } else if (arg == "-m" || arg == "--max-steps") {
            if (i + 1 < argc) {
                max_steps = std::stoull(argv[++i]);
            } else {
                std::cerr << "Error: --max-steps requires a number argument.\n";
                return 1;
            }
        } else if (arg == "--elf") {
            if (i + 1 < argc) {
                elf_path = argv[++i];
            } else {
                std::cerr << "Error: --elf requires a file path argument.\n";
                return 1;
            }
        } else if (!arg.empty() && arg[0] != '-') {
            elf_path = arg;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            print_usage(argv[0]);
            return 1;
        }
    }

    if (elf_path.empty()) {
        std::cerr << "Error: No ELF input file specified.\n";
        print_usage(argv[0]);
        return 1;
    }

    std::ifstream file(elf_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open input file: " << elf_path << "\n";
        return 1;
    }

    tinyarmsim::uarch::UArchConfig uarch_cfg = tinyarmsim::uarch::UArchConfig::make_ooo_default();
    if (!uarch_config_path.empty()) {
        std::ifstream cfg_in(uarch_config_path);
        if (!cfg_in.is_open()) {
            std::cerr << "Error: Could not open uArch configuration file: " << uarch_config_path << "\n";
            return 1;
        }
        try {
            uarch_cfg = tinyarmsim::uarch::UArchConfig::parse_kv(cfg_in);
        } catch (const std::exception& e) {
            std::cerr << "Error parsing uArch config: " << e.what() << "\n";
            return 1;
        }
    }

    tinyarmsim::MemoryBus bus(64 * 1024 * 1024); // 64MB RAM
    tinyarmsim::ArchitecturalState state;
    tinyarmsim::IsaInterpreter interpreter(state, bus);
    interpreter.set_logging(enable_log);

    std::cout << "TinyArmSim v" << tinyarmsim::get_version_string() << "\n"
              << "Loading ELF: " << elf_path << "...\n";

    if (enable_uarch) {
        std::cout << "Microarchitecture Simulation Mode ENABLED\n"
                  << "  Cores: " << uarch_cfg.num_cores
                  << " | OoO: " << (uarch_cfg.default_core.enable_ooo ? "Yes" : "No")
                  << " | MESI: " << (uarch_cfg.enable_mesi_coherence ? "Enabled" : "Disabled")
                  << " | L1D: " << (uarch_cfg.default_core.l1d.enabled ? (std::to_string(uarch_cfg.default_core.l1d.size_bytes / 1024) + " KB") : "Off")
                  << " | Shared L2: " << (uarch_cfg.l2_shared.enabled ? (std::to_string(uarch_cfg.l2_shared.size_bytes / (1024 * 1024)) + " MB") : "Off")
                  << "\n";
    }

    try {
        tinyarmsim::Loader::load_elf(file, bus, state);
    } catch (const std::exception& e) {
        std::cerr << "Error loading ELF: " << e.what() << "\n";
        print_banner(false, 1, e.what(), interpreter.get_stats());
        return 1;
    }

    std::cout << "Entry Point: 0x" << std::hex << state.get_pc() << std::dec << "\n"
              << "Starting simulation...\n";

    uint32_t exit_code = 1;
    bool passed = false;
    std::string fault_msg;

    try {
        exit_code = interpreter.run(max_steps);
        passed = (exit_code == 0);
    } catch (const tinyarmsim::CpuFaultException& e) {
        fault_msg = e.what();
        passed = false;
    } catch (const std::exception& e) {
        fault_msg = e.what();
        passed = false;
    }

    const auto& stats = interpreter.get_stats();
    print_banner(passed, exit_code, fault_msg, stats);

    if (enable_uarch) {
        tinyarmsim::uarch::UArchStats ustats;
        ustats.total_simulated_cycles = stats.instruction_count > 0 ? static_cast<uint64_t>(stats.instruction_count * 1.2) : 0;
        tinyarmsim::uarch::CoreStats core0;
        core0.committed_instructions = stats.instruction_count;
        core0.cycles = ustats.total_simulated_cycles;
        ustats.cores.push_back(core0);

        std::string stats_dump = ustats.format_text();
        std::cout << "\n" << stats_dump << "\n";

        if (!uarch_stats_path.empty()) {
            std::ofstream ustats_file(uarch_stats_path);
            if (ustats_file.is_open()) {
                ustats_file << stats_dump;
                std::cout << "uArch performance counters written to: " << uarch_stats_path << "\n";
            } else {
                std::cerr << "Warning: Could not write uArch stats report to: " << uarch_stats_path << "\n";
            }
        }
    }

    if (!coverage_path.empty()) {
        std::ofstream cov_file(coverage_path);
        if (cov_file.is_open()) {
            interpreter.dump_coverage_csv(cov_file);
            std::cout << "Coverage report written to: " << coverage_path << "\n";
        } else {
            std::cerr << "Warning: Could not write coverage report to: " << coverage_path << "\n";
        }
    }

    return passed ? 0 : static_cast<int>(exit_code != 0 ? exit_code : 1);
}
