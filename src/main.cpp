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

namespace {

void print_usage(const char* prog_name) {
    std::cout << "TinyArmSim v" << tinyarmsim::get_version_string() << " - ARM CPU ISA Simulator\n\n"
              << "Usage: " << prog_name << " [options] <elf-file>\n"
              << "Options:\n"
              << "  --elf <file>          Specify input ELF binary file\n"
              << "  -l, --log, --verbose  Enable step-by-step instruction trace logging\n"
              << "  -c, --coverage <file> Export instruction opcode coverage report to CSV\n"
              << "  -m, --max-steps <N>   Set maximum instruction execution steps (default: 5000000)\n"
              << "  -h, --help            Display this help message\n"
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
    bool enable_log = false;
    uint64_t max_steps = 5000000;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "-l" || arg == "--log" || arg == "--verbose") {
            enable_log = true;
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

    tinyarmsim::MemoryBus bus(64 * 1024 * 1024); // 64MB RAM
    tinyarmsim::ArchitecturalState state;
    tinyarmsim::IsaInterpreter interpreter(state, bus);
    interpreter.set_logging(enable_log);

    std::cout << "TinyArmSim v" << tinyarmsim::get_version_string() << "\n"
              << "Loading ELF: " << elf_path << "...\n";

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
