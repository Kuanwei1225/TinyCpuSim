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
    uint64_t mshr_allocations{0};
    uint64_t mshr_full_stalls{0};

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
    
    // Detailed Branch Breakdown
    uint64_t direct_cond{0};
    uint64_t direct_uncond{0};
    uint64_t calls{0};
    uint64_t returns{0};
    uint64_t indirects{0};

    // Sub-structures
    uint64_t btb_hits{0};
    uint64_t btb_misses{0};
    uint64_t ras_pushes{0};
    uint64_t ras_pops{0};
    uint64_t ras_hits{0};
    uint64_t ras_misses{0};
    uint64_t tage_hits{0};

    // gem5 Aligned Detailed Misprediction Breakdown
    uint64_t mispredict_due_to_direction{0};
    uint64_t mispredict_due_to_btb_miss{0};
    uint64_t squashed_branches{0};

    [[nodiscard]] double accuracy() const noexcept {
        return predictions > 0 ? static_cast<double>(correct_predictions) / static_cast<double>(predictions) : 1.0;
    }

    [[nodiscard]] double mispredict_rate() const noexcept {
        return predictions > 0 ? static_cast<double>(mispredictions) / static_cast<double>(predictions) : 0.0;
    }

    [[nodiscard]] double btb_hit_rate() const noexcept {
        uint64_t total = btb_hits + btb_misses;
        return total > 0 ? static_cast<double>(btb_hits) / static_cast<double>(total) : 0.0;
    }

    [[nodiscard]] double ras_hit_rate() const noexcept {
        uint64_t total = ras_hits + ras_misses;
        return total > 0 ? static_cast<double>(ras_hits) / static_cast<double>(total) : 0.0;
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
    uint64_t forward_stalls_pending_data{0};
    uint64_t memory_order_violations{0};
    uint64_t lq_full_stalls{0};
    uint64_t sq_full_stalls{0};

    [[nodiscard]] double forwarding_rate() const noexcept {
        return loads > 0 ? static_cast<double>(forwarded_loads) / static_cast<double>(loads) : 0.0;
    }
};

struct TopDownSummary {
    uint64_t total_slots{0};
    uint64_t retiring_slots{0};
    uint64_t bad_spec_slots{0};
    uint64_t frontend_slots{0};
    uint64_t backend_slots{0};

    // Level 2 Sub-metrics
    uint64_t retiring_base_alu{0};
    uint64_t retiring_mem{0};
    uint64_t fe_l1i_miss{0};
    uint64_t fe_fetch_bubble{0};
    uint64_t be_core_rs_full{0};
    uint64_t be_core_rob_full{0};
    uint64_t be_core_freelist_empty{0};
    uint64_t be_mem_l1d_miss{0};
    uint64_t be_mem_mshr_full{0};
    uint64_t be_mem_l2_miss{0};
    uint64_t be_mem_store_buf_full{0};

    [[nodiscard]] double retiring_pct() const noexcept {
        return total_slots > 0 ? (static_cast<double>(retiring_slots) / static_cast<double>(total_slots)) * 100.0 : 0.0;
    }
    [[nodiscard]] double bad_spec_pct() const noexcept {
        return total_slots > 0 ? (static_cast<double>(bad_spec_slots) / static_cast<double>(total_slots)) * 100.0 : 0.0;
    }
    [[nodiscard]] double frontend_pct() const noexcept {
        return total_slots > 0 ? (static_cast<double>(frontend_slots) / static_cast<double>(total_slots)) * 100.0 : 0.0;
    }
    [[nodiscard]] double backend_pct() const noexcept {
        return total_slots > 0 ? (static_cast<double>(backend_slots) / static_cast<double>(total_slots)) * 100.0 : 0.0;
    }
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
    uint64_t head_of_rob_stalls{0};

    // Execution Port Counters
    uint64_t port_alu_uops{0};
    uint64_t port_mul_uops{0};
    uint64_t port_div_uops{0};
    uint64_t port_branch_uops{0};
    uint64_t port_lsu_uops{0};

    CacheStats l1i{};
    CacheStats l1d{};
    BranchStats branch{};
    LsuStats lsu{};
    TopDownSummary topdown{};

    [[nodiscard]] double ipc() const noexcept {
        return cycles > 0 ? static_cast<double>(committed_instructions) / static_cast<double>(cycles) : 0.0;
    }

    [[nodiscard]] double uop_ipc() const noexcept {
        return cycles > 0 ? static_cast<double>(committed_uops) / static_cast<double>(cycles) : 0.0;
    }

    [[nodiscard]] double uop_expansion_ratio() const noexcept {
        return committed_instructions > 0 ? static_cast<double>(committed_uops) / static_cast<double>(committed_instructions) : 1.0;
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

    [[nodiscard]] uint64_t total_committed_uops() const noexcept {
        uint64_t sum = 0;
        for (const auto& c : cores) sum += c.committed_uops;
        return sum;
    }

    [[nodiscard]] double total_ipc() const noexcept {
        return total_simulated_cycles > 0 
            ? static_cast<double>(total_committed_instructions()) / static_cast<double>(total_simulated_cycles) 
            : 0.0;
    }

    [[nodiscard]] double total_uop_ipc() const noexcept {
        return total_simulated_cycles > 0 
            ? static_cast<double>(total_committed_uops()) / static_cast<double>(total_simulated_cycles) 
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

    [[nodiscard]] std::string format_text(bool all_perf = false) const {
        std::ostringstream oss;
        oss << "============================================================\n"
            << "               TinyCpuSim uArch Simulation Report           \n"
            << "============================================================\n"
            << "Simulated Target Clock:    " << std::fixed << std::setprecision(2) << target_frequency_mhz << " MHz\n"
            << "Simulated Target Time:     " << std::scientific << std::setprecision(4) << simulated_time_seconds() << " s\n"
            << "Simulated Total Cycles:    " << std::fixed << total_simulated_cycles << " ticks\n"
            << "Total Committed Insts:     " << total_committed_instructions() << "\n"
            << "Total Committed uOps:      " << total_committed_uops() << "\n"
            << std::fixed << std::setprecision(3)
            << "Aggregate Throughput (IPC):" << total_ipc() << " inst/cycle (uOp IPC: " << total_uop_ipc() << ")\n"
            << "Active Core Count:         " << cores.size() << "\n"
            << "------------------------------------------------------------\n"
            << "[ Host Simulator Performance ]\n"
            << "  Host Wall-Clock Time:    " << std::fixed << std::setprecision(6) << wall_time_seconds << " s\n"
            << "  Simulation Speed:        " << std::fixed << std::setprecision(2) << (simulation_speed_ticks_per_sec() / 1e6) << " M-Ticks/s\n"
            << "  Simulation MIPS:         " << std::fixed << std::setprecision(2) << simulation_mips() << " MIPS\n"
            << "  Simulation Slowdown:     " << std::fixed << std::setprecision(1) << slowdown_ratio() << "x\n";

        for (size_t i = 0; i < cores.size(); ++i) {
            const auto& c = cores[i];
            oss << "------------------------------------------------------------\n"
                << "[ Core " << i << " Summary ]\n"
                << "  Committed Insts / uOps:  " << c.committed_instructions << " / " << c.committed_uops 
                << " (uOp Ratio: " << std::fixed << std::setprecision(2) << c.uop_expansion_ratio() << "x)\n"
                << "  Core Throughput (IPC):   " << std::fixed << std::setprecision(3) << c.ipc() << " inst/cycle\n"
                << "  Branch Predictions:      " << c.branch.predictions << " (Accuracy: " << std::fixed << std::setprecision(1) << (c.branch.accuracy() * 100.0) << "%)\n"
                << "  Branch Mispredicts:      " << c.branch.mispredictions << " (Penalty Flushes: " << c.branch_mispredict_flushes << ")\n"
                << "  L1I Cache Hit Rate:      " << std::fixed << std::setprecision(2) << (c.l1i.hit_rate() * 100.0) << "% (" << c.l1i.hits << "/" << c.l1i.accesses << ")\n"
                << "  L1D Cache Hit Rate:      " << std::fixed << std::setprecision(2) << (c.l1d.hit_rate() * 100.0) << "% (" << c.l1d.hits << "/" << c.l1d.accesses << ")\n"
                << "  Loads / Stores:          " << c.lsu.loads << " / " << c.lsu.stores << "\n"
                << "  Store-to-Load Forwards:  " << c.lsu.forwarded_loads << " (Rate: " << (c.lsu.forwarding_rate() * 100.0) << "%)\n"
                << "  Mem Order Violations:    " << c.lsu.memory_order_violations << "\n";

            if (c.topdown.total_slots > 0) {
                oss << "  --- Top-Down Breakdown (Level 1) ---\n"
                    << "    Frontend Bound:        " << std::fixed << std::setprecision(2) << c.topdown.frontend_pct() << "%\n"
                    << "    Bad Speculation:       " << std::fixed << std::setprecision(2) << c.topdown.bad_spec_pct() << "%\n"
                    << "    Backend Bound:         " << std::fixed << std::setprecision(2) << c.topdown.backend_pct() << "%\n"
                    << "    Retiring:              " << std::fixed << std::setprecision(2) << c.topdown.retiring_pct() << "%\n";
            }

            if (all_perf) {
                oss << "  --- Detailed Execution Ports ---\n"
                    << "    Port ALU uOps:         " << c.port_alu_uops << "\n"
                    << "    Port MUL uOps:         " << c.port_mul_uops << "\n"
                    << "    Port DIV uOps:         " << c.port_div_uops << "\n"
                    << "    Port Branch uOps:      " << c.port_branch_uops << "\n"
                    << "    Port LSU uOps:         " << c.port_lsu_uops << "\n"
                    << "  --- Pipeline Stalls Breakdown ---\n"
                    << "    ROB Full Stalls:       " << c.rob_full_stalls << " cycles\n"
                    << "    RS/IQ Full Stalls:     " << c.rs_full_stalls << " cycles\n"
                    << "    PRF Exhaustion Stalls: " << c.rename_reg_exhaustion_stalls << " cycles\n"
                    << "    LQ / SQ Full Stalls:   " << c.lq_full_stalls << " / " << c.sq_full_stalls << " cycles\n"
                    << "    Head-of-ROB Stalls:    " << c.head_of_rob_stalls << " cycles\n"
                    << "  --- Branch Predictor Diagnostics ---\n"
                    << "    Direct Cond / Uncond:  " << c.branch.direct_cond << " / " << c.branch.direct_uncond << "\n"
                    << "    Calls / Returns:       " << c.branch.calls << " / " << c.branch.returns << "\n"
                    << "    Indirect Branches:     " << c.branch.indirects << "\n"
                    << "    Mispredict Dir / BTB:  " << c.branch.mispredict_due_to_direction << " / " << c.branch.mispredict_due_to_btb_miss << "\n"
                    << "    Squashed Spec Branches:" << c.branch.squashed_branches << "\n"
                    << "    BTB Hits / Misses:     " << c.branch.btb_hits << " / " << c.branch.btb_misses << " (Hit Rate: " << (c.branch.btb_hit_rate() * 100.0) << "%)\n"
                    << "    RAS Hits / Misses:     " << c.branch.ras_hits << " / " << c.branch.ras_misses << " (Hit Rate: " << (c.branch.ras_hit_rate() * 100.0) << "%)\n"
                    << "    RAS Pushes / Pops:     " << c.branch.ras_pushes << " / " << c.branch.ras_pops << "\n"
                    << "    TAGE Provider Hits:    " << c.branch.tage_hits << "\n"
                    << "  --- LSU & Memory Disambiguation ---\n"
                    << "    Store-to-Load Forwards:" << c.lsu.forwarded_loads << "\n"
                    << "    Store Data Replays:    " << c.lsu.forward_stalls_pending_data << "\n"
                    << "    Memory Order Flushes:  " << c.lsu.memory_order_violations << "\n"
                    << "  --- Cache & MSHR Subsystem ---\n"
                    << "    L1I Misses / Evictions:" << c.l1i.misses << " / " << c.l1i.evictions << "\n"
                    << "    L1D Misses / Evictions:" << c.l1d.misses << " / " << c.l1d.evictions << "\n"
                    << "    L1D Dirty Writebacks:  " << c.l1d.writebacks << "\n"
                    << "    L1D MSHR Allocations:  " << c.l1d.mshr_allocations << " (Stalls: " << c.l1d.mshr_full_stalls << ")\n";

                if (c.topdown.total_slots > 0) {
                    oss << "  --- Top-Down Breakdown (Level 2) ---\n"
                        << "    Retiring Base ALU:     " << c.topdown.retiring_base_alu << " slots\n"
                        << "    Retiring Mem:          " << c.topdown.retiring_mem << " slots\n"
                        << "    FE L1I Miss:           " << c.topdown.fe_l1i_miss << " slots\n"
                        << "    FE Fetch Bubbles:      " << c.topdown.fe_fetch_bubble << " slots\n"
                        << "    BE Core RS Full:       " << c.topdown.be_core_rs_full << " slots\n"
                        << "    BE Core ROB Full:      " << c.topdown.be_core_rob_full << " slots\n"
                        << "    BE Core FreeList Empty:" << c.topdown.be_core_freelist_empty << " slots\n"
                        << "    BE Mem L1D Miss:       " << c.topdown.be_mem_l1d_miss << " slots\n"
                        << "    BE Mem MSHR Full:      " << c.topdown.be_mem_mshr_full << " slots\n"
                        << "    BE Mem L2 Miss:        " << c.topdown.be_mem_l2_miss << " slots\n"
                        << "    BE Mem StoreBuf Full:  " << c.topdown.be_mem_store_buf_full << " slots\n";
                }
            }
        }

        oss << "------------------------------------------------------------\n"
            << "[ Shared L2 Cache & Coherence ]\n"
            << "  L2 Accesses:             " << l2_shared.accesses << "\n"
            << "  L2 Hits / Misses:        " << l2_shared.hits << " / " << l2_shared.misses 
            << " (Hit Rate: " << std::fixed << std::setprecision(2) << (l2_shared.hit_rate() * 100.0) << "%)\n"
            << "  L2 Evictions / WB:       " << l2_shared.evictions << " / " << l2_shared.writebacks << "\n"
            << "  MESI Snoop Broadcasts:   " << mesi_snoop_requests << "\n"
            << "  MESI Line Invalidations: " << mesi_invalidations << "\n"
            << "============================================================\n";

        return oss.str();
    }

    [[nodiscard]] std::string format_json() const {
        std::ostringstream oss;
        oss << "{\n"
            << "  \"simulated_cycles\": " << total_simulated_cycles << ",\n"
            << "  \"committed_instructions\": " << total_committed_instructions() << ",\n"
            << "  \"committed_uops\": " << total_committed_uops() << ",\n"
            << "  \"ipc\": " << total_ipc() << ",\n"
            << "  \"uop_ipc\": " << total_uop_ipc() << ",\n"
            << "  \"wall_time_seconds\": " << wall_time_seconds << ",\n"
            << "  \"simulation_mips\": " << simulation_mips() << ",\n"
            << "  \"cores\": [\n";

        for (size_t i = 0; i < cores.size(); ++i) {
            const auto& c = cores[i];
            oss << "    {\n"
                << "      \"core_id\": " << i << ",\n"
                << "      \"cycles\": " << c.cycles << ",\n"
                << "      \"committed_insts\": " << c.committed_instructions << ",\n"
                << "      \"committed_uops\": " << c.committed_uops << ",\n"
                << "      \"ipc\": " << c.ipc() << ",\n"
                << "      \"branch_predictions\": " << c.branch.predictions << ",\n"
                << "      \"branch_accuracy\": " << c.branch.accuracy() << ",\n"
                << "      \"branch_mispredicts\": " << c.branch.mispredictions << ",\n"
                << "      \"l1i_hit_rate\": " << c.l1i.hit_rate() << ",\n"
                << "      \"l1d_hit_rate\": " << c.l1d.hit_rate() << ",\n"
                << "      \"loads\": " << c.lsu.loads << ",\n"
                << "      \"stores\": " << c.lsu.stores << ",\n"
                << "      \"forwarded_loads\": " << c.lsu.forwarded_loads << ",\n"
                << "      \"memory_order_violations\": " << c.lsu.memory_order_violations << ",\n"
                << "      \"topdown\": {\n"
                << "        \"frontend_pct\": " << c.topdown.frontend_pct() << ",\n"
                << "        \"bad_spec_pct\": " << c.topdown.bad_spec_pct() << ",\n"
                << "        \"backend_pct\": " << c.topdown.backend_pct() << ",\n"
                << "        \"retiring_pct\": " << c.topdown.retiring_pct() << "\n"
                << "      }\n"
                << "    }" << (i + 1 < cores.size() ? "," : "") << "\n";
        }

        oss << "  ],\n"
            << "  \"l2_shared\": {\n"
            << "    \"accesses\": " << l2_shared.accesses << ",\n"
            << "    \"hits\": " << l2_shared.hits << ",\n"
            << "    \"misses\": " << l2_shared.misses << ",\n"
            << "    \"hit_rate\": " << l2_shared.hit_rate() << "\n"
            << "  }\n"
            << "}\n";
        return oss.str();
    }
};

} // namespace tinyarmsim::uarch
