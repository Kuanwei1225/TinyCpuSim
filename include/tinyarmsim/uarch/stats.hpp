#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <iomanip>

namespace tinyarmsim::uarch {

struct CacheStats {
    uint64_t accesses{0};
    uint64_t hits{0};
    uint64_t misses{0};
    uint64_t evictions{0};
    uint64_t writebacks{0};
    uint64_t invalidations{0};

    [[nodiscard]] double hit_rate() const noexcept {
        return accesses > 0 ? static_cast<double>(hits) / static_cast<double>(accesses) : 0.0;
    }

    [[nodiscard]] double miss_rate() const noexcept {
        return accesses > 0 ? static_cast<double>(misses) / static_cast<double>(accesses) : 0.0;
    }

    void record_access(bool is_hit) noexcept {
        accesses++;
        if (is_hit) hits++;
        else misses++;
    }
};

struct BranchStats {
    uint64_t predictions{0};
    uint64_t correct_predictions{0};
    uint64_t mispredictions{0};
    uint64_t btb_hits{0};
    uint64_t ras_hits{0};

    [[nodiscard]] double accuracy() const noexcept {
        return predictions > 0 ? static_cast<double>(correct_predictions) / static_cast<double>(predictions) : 1.0;
    }

    [[nodiscard]] double mispredict_rate() const noexcept {
        return predictions > 0 ? static_cast<double>(mispredictions) / static_cast<double>(predictions) : 0.0;
    }

    void record_prediction(bool correct) noexcept {
        predictions++;
        if (correct) correct_predictions++;
        else mispredictions++;
    }
};

struct LsuStats {
    uint64_t loads{0};
    uint64_t stores{0};
    uint64_t forwarded_loads{0};
    uint64_t memory_order_violations{0};
};

struct CoreStats {
    uint64_t cycles{0};
    uint64_t committed_instructions{0};
    uint64_t committed_uops{0};
    
    // Stalls breakdown
    uint64_t rob_full_stalls{0};
    uint64_t rs_full_stalls{0};
    uint64_t lq_full_stalls{0};
    uint64_t sq_full_stalls{0};
    uint64_t rename_reg_exhaustion_stalls{0};
    uint64_t branch_mispredict_flushes{0};

    CacheStats l1i{};
    CacheStats l1d{};
    BranchStats branch{};
    LsuStats lsu{};

    [[nodiscard]] double ipc() const noexcept {
        return cycles > 0 ? static_cast<double>(committed_instructions) / static_cast<double>(cycles) : 0.0;
    }

    [[nodiscard]] double uop_ipc() const noexcept {
        return cycles > 0 ? static_cast<double>(committed_uops) / static_cast<double>(cycles) : 0.0;
    }
};

struct UArchStats {
    uint64_t total_simulated_cycles{0};
    double target_frequency_mhz{1000.0}; // Default 1.0 GHz (1 cycle = 1.0 ns)
    double wall_time_seconds{0.0};
    std::vector<CoreStats> cores{};
    CacheStats l2_shared{};
    uint64_t mesi_snoop_requests{0};
    uint64_t mesi_invalidations{0};

    [[nodiscard]] uint64_t total_committed_instructions() const noexcept {
        uint64_t sum = 0;
        for (const auto& c : cores) sum += c.committed_instructions;
        return sum;
    }

    [[nodiscard]] double total_ipc() const noexcept {
        return total_simulated_cycles > 0 
            ? static_cast<double>(total_committed_instructions()) / static_cast<double>(total_simulated_cycles) 
            : 0.0;
    }

    [[nodiscard]] double simulated_time_seconds() const noexcept {
        return target_frequency_mhz > 0.0
            ? static_cast<double>(total_simulated_cycles) / (target_frequency_mhz * 1e6)
            : 0.0;
    }

    [[nodiscard]] double simulation_speed_ticks_per_sec() const noexcept {
        return wall_time_seconds > 0.0
            ? static_cast<double>(total_simulated_cycles) / wall_time_seconds
            : 0.0;
    }

    [[nodiscard]] double simulation_mips() const noexcept {
        return wall_time_seconds > 0.0
            ? static_cast<double>(total_committed_instructions()) / (wall_time_seconds * 1e6)
            : 0.0;
    }

    [[nodiscard]] double slowdown_ratio() const noexcept {
        double sim_t = simulated_time_seconds();
        return sim_t > 0.0 ? wall_time_seconds / sim_t : 0.0;
    }

    [[nodiscard]] std::string format_text() const {
        std::ostringstream oss;
        oss << "============================================================\n"
            << "               TinyArmSim uArch Simulation Report           \n"
            << "============================================================\n"
            << "Simulated Target Clock:    " << std::fixed << std::setprecision(2) << target_frequency_mhz << " MHz\n"
            << "Simulated Target Time:     " << std::scientific << std::setprecision(4) << simulated_time_seconds() << " s\n"
            << "Simulated Total Cycles:    " << std::fixed << total_simulated_cycles << " ticks\n"
            << "Total Committed Insts:     " << total_committed_instructions() << "\n"
            << std::fixed << std::setprecision(3)
            << "Aggregate Throughput (IPC):" << total_ipc() << " inst/cycle\n"
            << "Active Core Count:         " << cores.size() << "\n"
            << "------------------------------------------------------------\n"
            << "[ Host Simulator Performance ]\n"
            << "  Host Wall-Clock Time:    " << std::fixed << std::setprecision(6) << wall_time_seconds << " s\n"
            << "  Simulation Speed (Ticks):" << std::fixed << std::setprecision(2) << simulation_speed_ticks_per_sec() << " ticks/s (" 
            << (simulation_speed_ticks_per_sec() / 1e6) << " M-Ticks/s)\n"
            << "  Simulation MIPS:         " << std::fixed << std::setprecision(2) << simulation_mips() << " MIPS\n"
            << "  Simulation Slowdown:     " << std::fixed << std::setprecision(1) << slowdown_ratio() << "x\n"
            << "------------------------------------------------------------\n";

        for (size_t i = 0; i < cores.size(); ++i) {
            const auto& c = cores[i];
            oss << "[ Core " << i << " Statistics ]\n"
                << "  Committed Instructions:  " << c.committed_instructions << "\n"
                << "  Core IPC:                " << c.ipc() << " inst/cycle\n"
                << "  Branch Predictions:      " << c.branch.predictions << " (Accuracy: " << (c.branch.accuracy() * 100.0) << "%)\n"
                << "  Branch Mispredicts:      " << c.branch.mispredictions << "\n"
                << "  L1I Cache Accesses:      " << c.l1i.accesses << " (Hit Rate: " << (c.l1i.hit_rate() * 100.0) << "%)\n"
                << "  L1D Cache Accesses:      " << c.l1d.accesses << " (Hit Rate: " << (c.l1d.hit_rate() * 100.0) << "%)\n"
                << "  Loads / Stores:          " << c.lsu.loads << " / " << c.lsu.stores << "\n"
                << "  Store-to-Load Forwards:  " << c.lsu.forwarded_loads << "\n"
                << "  Mem Order Violations:    " << c.lsu.memory_order_violations << "\n"
                << "  ROB / RS Full Stalls:    " << c.rob_full_stalls << " / " << c.rs_full_stalls << "\n";
        }

        oss << "------------------------------------------------------------\n"
            << "[ Shared L2 Cache & Coherence ]\n"
            << "  L2 Accesses:             " << l2_shared.accesses << "\n"
            << "  L2 Hits / Misses:        " << l2_shared.hits << " / " << l2_shared.misses 
            << " (Hit Rate: " << (l2_shared.hit_rate() * 100.0) << "%)\n"
            << "  L2 Writebacks:           " << l2_shared.writebacks << "\n"
            << "  MESI Snoop Broadcasts:   " << mesi_snoop_requests << "\n"
            << "  MESI Line Invalidations: " << mesi_invalidations << "\n"
            << "============================================================\n";

        return oss.str();
    }
};

} // namespace tinyarmsim::uarch
