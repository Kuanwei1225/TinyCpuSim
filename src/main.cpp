#include <iostream>
#include <fstream>
#include "tinyarmsim/common.hpp"
#include "tinyarmsim/state.hpp"
#include "tinyarmsim/memory_bus.hpp"
#include "tinyarmsim/loader.hpp"
#include "tinyarmsim/interpreter.hpp"

int main(int argc, char* argv[]) {
    std::cout << "TinyArmSim v" << tinyarmsim::get_version_string() << " (ARM CPU Simulator)" << std::endl;

    if (argc < 2) {
        std::cout << "Usage: tinyarmsim <path-to-elf-or-bin>" << std::endl;
        return 0;
    }

    std::string file_path = argv[1];
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open input file: " << file_path << std::endl;
        return 1;
    }

    try {
        tinyarmsim::MemoryBus bus(64 * 1024 * 1024); // 64MB RAM
        tinyarmsim::ArchitecturalState state;
        tinyarmsim::IsaInterpreter interpreter(state, bus);

        std::cout << "Loading ELF binary: " << file_path << "..." << std::endl;
        tinyarmsim::Loader::load_elf(file, bus, state);
        std::cout << "Entry Point: 0x" << std::hex << state.get_pc() << std::dec << std::endl;

        std::cout << "Starting simulation..." << std::endl;
        uint32_t exit_code = interpreter.run(5000000);
        std::cout << "Simulation completed with exit code: " << exit_code
                  << " (R0 = " << exit_code << ")" << std::endl;
        return static_cast<int>(exit_code);
    } catch (const std::exception& e) {
        std::cerr << "Simulation Fault: " << e.what() << std::endl;
        return 2;
    }
}
