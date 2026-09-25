#include <iostream>
#include "tinyarmsim/common.hpp"

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    std::cout << "TinyArmSim v" << tinyarmsim::get_version_string() << " initialized." << std::endl;
    return 0;
}
