#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <fstream>

namespace tinyarmsim::uarch {

enum class SlotType : uint8_t {
    RetiringBaseAlu,
    RetiringMem,
    BadSpecMispredict,
    FrontEndL1IMiss,
    FrontEndFetchBubble,
    BackEndCoreRSFull,
    BackEndCoreRobFull,
    BackEndCoreFreeListEmpty,
    BackEndMemL1DMiss,
    BackEndMemMSHRFull,
    BackEndMemL2Miss,
    BackEndMemStoreBufFull
};

enum class PerfEvent : uint8_t {
    StoreLoadForwardHit,
    MemoryOrderViolation,
    MSHRFullStall,
    MESIInvalidation,
    BranchMispredict
};

enum class ExportFormat : uint8_t {
    Text,
    JSON,
    CSV
};

enum class RoiState : uint8_t {
    Inactive,
    Active,
    Snapshot
};

struct BottleneckEntry {
    std::string name;
    uint64_t slots{0};
    double percentage{0.0};
};

struct TopDownCoreReport {
    size_t core_id{0};
    uint64_t total_slots{0};

    // Level-1
    uint64_t retiring_slots{0};
    uint64_t bad_spec_slots{0};
    uint64_t frontend_slots{0};
    uint64_t backend_slots{0};

    // Level-2 Retiring
    uint64_t retiring_base_alu{0};
    uint64_t retiring_mem{0};

    // Level-2 FrontEnd
    uint64_t fe_l1i_miss{0};
    uint64_t fe_fetch_bubble{0};

    // Level-2 BackEnd Core
    uint64_t be_core_rs_full{0};
    uint64_t be_core_rob_full{0};
    uint64_t be_core_freelist_empty{0};

    // Level-2 BackEnd Memory
    uint64_t be_mem_l1d_miss{0};
    uint64_t be_mem_mshr_full{0};
    uint64_t be_mem_l2_miss{0};
    uint64_t be_mem_store_buf_full{0};

    // Microarchitectural events
    uint64_t event_store_forward_hits{0};
    uint64_t event_mob_violations{0};
    uint64_t event_mshr_stalls{0};
    uint64_t event_mesi_invalidations{0};
    uint64_t event_branch_mispredicts{0};

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

class TopDownProfiler {
public:
    TopDownProfiler() = default;

    void init(size_t num_cores = 1, uint32_t issue_width = 2) {
        num_cores_ = num_cores;
        issue_width_ = issue_width;
        core_reports_.assign(num_cores, TopDownCoreReport{});
        for (size_t i = 0; i < num_cores; ++i) {
            core_reports_[i].core_id = i;
        }
        roi_state_ = RoiState::Active; // By default active unless dynamic ROI is configured
    }

    void start_roi() noexcept {
        roi_state_ = RoiState::Active;
    }

    void stop_roi() noexcept {
        roi_state_ = RoiState::Snapshot;
    }

    void reset_stats() noexcept {
        for (size_t i = 0; i < core_reports_.size(); ++i) {
            core_reports_[i] = TopDownCoreReport{};
            core_reports_[i].core_id = i;
        }
    }

    [[nodiscard]] bool is_roi_active() const noexcept {
        return roi_state_ == RoiState::Active;
    }

    void record_slot(size_t core_id, SlotType slot) noexcept {
        if (roi_state_ != RoiState::Active || core_id >= core_reports_.size()) return;
        auto& r = core_reports_[core_id];
        r.total_slots++;

        switch (slot) {
            case SlotType::RetiringBaseAlu:
                r.retiring_slots++;
                r.retiring_base_alu++;
                break;
            case SlotType::RetiringMem:
                r.retiring_slots++;
                r.retiring_mem++;
                break;
            case SlotType::BadSpecMispredict:
                r.bad_spec_slots++;
                break;
            case SlotType::FrontEndL1IMiss:
                r.frontend_slots++;
                r.fe_l1i_miss++;
                break;
            case SlotType::FrontEndFetchBubble:
                r.frontend_slots++;
                r.fe_fetch_bubble++;
                break;
            case SlotType::BackEndCoreRSFull:
                r.backend_slots++;
                r.be_core_rs_full++;
                break;
            case SlotType::BackEndCoreRobFull:
                r.backend_slots++;
                r.be_core_rob_full++;
                break;
            case SlotType::BackEndCoreFreeListEmpty:
                r.backend_slots++;
                r.be_core_freelist_empty++;
                break;
            case SlotType::BackEndMemL1DMiss:
                r.backend_slots++;
                r.be_mem_l1d_miss++;
                break;
            case SlotType::BackEndMemMSHRFull:
                r.backend_slots++;
                r.be_mem_mshr_full++;
                break;
            case SlotType::BackEndMemL2Miss:
                r.backend_slots++;
                r.be_mem_l2_miss++;
                break;
            case SlotType::BackEndMemStoreBufFull:
                r.backend_slots++;
                r.be_mem_store_buf_full++;
                break;
        }
    }

    void record_event(size_t core_id, PerfEvent event) noexcept {
        if (roi_state_ != RoiState::Active || core_id >= core_reports_.size()) return;
        auto& r = core_reports_[core_id];
        switch (event) {
            case PerfEvent::StoreLoadForwardHit:
                r.event_store_forward_hits++;
                break;
            case PerfEvent::MemoryOrderViolation:
                r.event_mob_violations++;
                break;
            case PerfEvent::MSHRFullStall:
                r.event_mshr_stalls++;
                break;
            case PerfEvent::MESIInvalidation:
                r.event_mesi_invalidations++;
                break;
            case PerfEvent::BranchMispredict:
                r.event_branch_mispredicts++;
                break;
        }
    }

    void trigger_m5op(uint32_t op) {
        if (op == 0x50) {
            // m5_reset_stats
            reset_stats();
            start_roi();
        } else if (op == 0x51) {
            // m5_dump_stats
            stop_roi();
        } else if (op == 0x52) {
            // m5_exit
            stop_roi();
        }
    }

    [[nodiscard]] TopDownCoreReport get_report(size_t core_id = 0) const {
        if (core_id < core_reports_.size()) return core_reports_[core_id];
        return TopDownCoreReport{};
    }

    [[nodiscard]] std::vector<BottleneckEntry> get_top_bottlenecks(size_t core_id = 0) const {
        if (core_id >= core_reports_.size()) return {};
        const auto& r = core_reports_[core_id];
        if (r.total_slots == 0) return {};

        std::vector<BottleneckEntry> entries;
        auto add = [&](const std::string& name, uint64_t slots) {
            if (slots > 0) {
                double pct = (static_cast<double>(slots) / static_cast<double>(r.total_slots)) * 100.0;
                entries.push_back({name, slots, pct});
            }
        };

        add("Back-End Memory / L1D Miss", r.be_mem_l1d_miss);
        add("Back-End Memory / MSHR Full", r.be_mem_mshr_full);
        add("Back-End Memory / L2 Miss", r.be_mem_l2_miss);
        add("Back-End Core / RS Full", r.be_core_rs_full);
        add("Back-End Core / ROB Full", r.be_core_rob_full);
        add("Back-End Core / FreeList Empty", r.be_core_freelist_empty);
        add("Front-End / L1I Miss", r.fe_l1i_miss);
        add("Front-End / Fetch Bubble", r.fe_fetch_bubble);
        add("Bad Speculation / Branch Mispredict", r.bad_spec_slots);

        std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
            return a.slots > b.slots;
        });
        return entries;
    }

    [[nodiscard]] std::string format_text() const {
        std::ostringstream oss;
        oss << "======================================================================\n"
            << "               TinyArmSim Top-Down Microarchitecture Report           \n"
            << "======================================================================\n";

        for (size_t i = 0; i < core_reports_.size(); ++i) {
            const auto& r = core_reports_[i];
            oss << "[ Core " << i << " Top-Down Breakdown ]\n"
                << "  Total Pipeline Slots: " << r.total_slots << " (" << issue_width_ << "-wide engine)\n"
                << "----------------------------------------------------------------------\n"
                << std::fixed << std::setprecision(1)
                << "  ├── Retiring (Effective Ops):       " << std::setw(8) << r.retiring_slots << " slots (" << std::setw(5) << r.retiring_pct() << "% )\n"
                << "  │     ├── Base ALU:                 " << std::setw(8) << r.retiring_base_alu << " slots\n"
                << "  │     └── Memory Operations:        " << std::setw(8) << r.retiring_mem << " slots\n"
                << "  ├── Bad Speculation (Squashed):     " << std::setw(8) << r.bad_spec_slots << " slots (" << std::setw(5) << r.bad_spec_pct() << "% )\n"
                << "  ├── Front-End Bound:                " << std::setw(8) << r.frontend_slots << " slots (" << std::setw(5) << r.frontend_pct() << "% )\n"
                << "  │     ├── L1I Miss Stall:           " << std::setw(8) << r.fe_l1i_miss << " slots\n"
                << "  │     └── Fetch/BTB Bubble:         " << std::setw(8) << r.fe_fetch_bubble << " slots\n"
                << "  └── Back-End Bound:                 " << std::setw(8) << r.backend_slots << " slots (" << std::setw(5) << r.backend_pct() << "% )\n"
                << "        ├── Core Bound (RS/ROB Full): " << std::setw(8) << (r.be_core_rs_full + r.be_core_rob_full + r.be_core_freelist_empty) << " slots\n"
                << "        └── Memory Bound (L1D/MSHR):  " << std::setw(8) << (r.be_mem_l1d_miss + r.be_mem_mshr_full + r.be_mem_l2_miss) << " slots\n"
                << "----------------------------------------------------------------------\n"
                << "  Microarchitectural Events:\n"
                << "    Store-to-Load Forward Hits:      " << r.event_store_forward_hits << "\n"
                << "    Memory Order Violations (MOB):   " << r.event_mob_violations << "\n"
                << "    MSHR Saturation Stalls:          " << r.event_mshr_stalls << "\n"
                << "----------------------------------------------------------------------\n";

            auto bottlenecks = get_top_bottlenecks(i);
            if (!bottlenecks.empty()) {
                oss << "  Top Bottlenecks (Pareto Analysis):\n";
                for (size_t b = 0; b < std::min<size_t>(3, bottlenecks.size()); ++b) {
                    oss << "    #" << (b + 1) << ". " << bottlenecks[b].name << " (" << std::fixed << std::setprecision(1) << bottlenecks[b].percentage << "% of total slots)\n";
                }
                oss << "----------------------------------------------------------------------\n";
            }
        }
        oss << "======================================================================\n";
        return oss.str();
    }

    [[nodiscard]] std::string format_json() const {
        std::ostringstream oss;
        oss << "{\n"
            << "  \"num_cores\": " << num_cores_ << ",\n"
            << "  \"issue_width\": " << issue_width_ << ",\n"
            << "  \"cores\": [\n";
        for (size_t i = 0; i < core_reports_.size(); ++i) {
            const auto& r = core_reports_[i];
            oss << "    {\n"
                << "      \"core_id\": " << r.core_id << ",\n"
                << "      \"total_slots\": " << r.total_slots << ",\n"
                << "      \"retiring_slots\": " << r.retiring_slots << ",\n"
                << "      \"bad_spec_slots\": " << r.bad_spec_slots << ",\n"
                << "      \"frontend_slots\": " << r.frontend_slots << ",\n"
                << "      \"backend_slots\": " << r.backend_slots << ",\n"
                << "      \"retiring_base_alu\": " << r.retiring_base_alu << ",\n"
                << "      \"retiring_mem\": " << r.retiring_mem << ",\n"
                << "      \"fe_l1i_miss\": " << r.fe_l1i_miss << ",\n"
                << "      \"fe_fetch_bubble\": " << r.fe_fetch_bubble << ",\n"
                << "      \"be_core_rs_full\": " << r.be_core_rs_full << ",\n"
                << "      \"be_core_rob_full\": " << r.be_core_rob_full << ",\n"
                << "      \"be_mem_l1d_miss\": " << r.be_mem_l1d_miss << ",\n"
                << "      \"be_mem_mshr_full\": " << r.be_mem_mshr_full << ",\n"
                << "      \"store_forward_hits\": " << r.event_store_forward_hits << ",\n"
                << "      \"mob_violations\": " << r.event_mob_violations << "\n"
                << "    }" << (i + 1 < core_reports_.size() ? "," : "") << "\n";
        }
        oss << "  ]\n}\n";
        return oss.str();
    }

    [[nodiscard]] std::string format_csv() const {
        std::ostringstream oss;
        oss << "core_id,total_slots,retiring,bad_spec,frontend,backend,be_mem_l1d_miss,be_core_rs_full,store_forward_hits\n";
        for (const auto& r : core_reports_) {
            oss << r.core_id << ","
                << r.total_slots << ","
                << r.retiring_slots << ","
                << r.bad_spec_slots << ","
                << r.frontend_slots << ","
                << r.backend_slots << ","
                << r.be_mem_l1d_miss << ","
                << r.be_core_rs_full << ","
                << r.event_store_forward_hits << "\n";
        }
        return oss.str();
    }

    void export_report(const std::string& path, ExportFormat fmt) const {
        std::ofstream ofs(path);
        if (!ofs.is_open()) return;
        if (fmt == ExportFormat::JSON) ofs << format_json();
        else if (fmt == ExportFormat::CSV) ofs << format_csv();
        else ofs << format_text();
    }

private:
    size_t num_cores_{1};
    uint32_t issue_width_{2};
    std::vector<TopDownCoreReport> core_reports_{};
    RoiState roi_state_{RoiState::Active};
};

} // namespace tinyarmsim::uarch
