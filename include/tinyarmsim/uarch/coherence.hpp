#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <stdexcept>
#include "tinyarmsim/uarch/config.hpp"
#include "tinyarmsim/uarch/cache.hpp"

namespace tinyarmsim::uarch {

enum class MESIState : uint8_t {
    INVALID = 0,
    SHARED = 1,
    EXCLUSIVE = 2,
    MODIFIED = 3
};

[[nodiscard]] constexpr const char* mesi_state_to_string(MESIState state) noexcept {
    switch (state) {
        case MESIState::INVALID: return "I";
        case MESIState::SHARED: return "S";
        case MESIState::EXCLUSIVE: return "E";
        case MESIState::MODIFIED: return "M";
    }
    return "?";
}

enum class BusTransactionType : uint8_t {
    NONE = 0,
    BUS_RD,     // Read request for shared line
    BUS_RDX,    // Read with intent to modify (requests exclusive copy and invalidates peers)
    BUS_UPGR,   // Upgrade from Shared to Modified (invalidates peers)
    BUS_WB      // Writeback of dirty line
};

[[nodiscard]] constexpr const char* bus_tx_to_string(BusTransactionType tx) noexcept {
    switch (tx) {
        case BusTransactionType::NONE: return "NONE";
        case BusTransactionType::BUS_RD: return "BusRd";
        case BusTransactionType::BUS_RDX: return "BusRdX";
        case BusTransactionType::BUS_UPGR: return "BusUpgr";
        case BusTransactionType::BUS_WB: return "BusWB";
    }
    return "UNKNOWN";
}

struct SnoopResponse {
    bool shared_asserted{false}; // Set if another core also has a valid copy of this line
    bool flushed_data{false};     // Set if a core in Modified state had to flush data to bus
    uint32_t flush_addr{0};
    uint64_t invalidations_caused{0};
};

struct CoherenceAction {
    MESIState old_state{MESIState::INVALID};
    MESIState new_state{MESIState::INVALID};
    BusTransactionType bus_tx{BusTransactionType::NONE};
    bool is_hit{false};
    uint32_t latency_cycles{1};
    uint64_t invalidations{0};
};

// Line-level MESI state container per core
struct CoreMESILine {
    uint32_t tag{0};
    MESIState state{MESIState::INVALID};
    uint64_t last_access{0};
};

struct CoreExclusiveMonitor {
    bool active{false};
    uint32_t line_addr{0};
};

class MESICoherenceEngine {
public:
    explicit MESICoherenceEngine(size_t num_cores = 1, size_t line_size = 64)
        : num_cores_(num_cores), line_size_(line_size) {
        if (num_cores_ == 0) num_cores_ = 1;
        offset_mask_ = static_cast<uint32_t>(line_size_ - 1);
        core_lines_.resize(num_cores_);
        monitors_.resize(num_cores_);
    }

    [[nodiscard]] size_t get_num_cores() const noexcept { return num_cores_; }
    [[nodiscard]] uint32_t align_to_line(uint32_t addr) const noexcept { return addr & ~offset_mask_; }

    [[nodiscard]] MESIState get_state(size_t core_id, uint32_t addr) const {
        if (core_id >= num_cores_) return MESIState::INVALID;
        uint32_t line_addr = align_to_line(addr);
        auto it = core_lines_[core_id].find(line_addr);
        if (it != core_lines_[core_id].end()) {
            return it->second.state;
        }
        return MESIState::INVALID;
    }

    void set_state(size_t core_id, uint32_t addr, MESIState state) {
        if (core_id >= num_cores_) return;
        uint32_t line_addr = align_to_line(addr);
        if (state == MESIState::INVALID) {
            core_lines_[core_id].erase(line_addr);
        } else {
            core_lines_[core_id][line_addr] = CoreMESILine{line_addr, state, access_seq_++};
        }
    }

    // Core Local Read Access: Determines hit/miss and generates BusRd if needed
    CoherenceAction handle_cpu_read(size_t core_id, uint32_t addr) {
        CoherenceAction act{};
        uint32_t line_addr = align_to_line(addr);
        MESIState current = get_state(core_id, line_addr);
        act.old_state = current;

        if (current == MESIState::MODIFIED || current == MESIState::EXCLUSIVE || current == MESIState::SHARED) {
            // Local Hit (no bus action needed)
            act.is_hit = true;
            act.new_state = current;
            act.bus_tx = BusTransactionType::NONE;
            act.latency_cycles = 1;
            return act;
        }

        // Local Miss (INVALID) -> Must broadcast BusRd
        act.is_hit = false;
        act.bus_tx = BusTransactionType::BUS_RD;
        
        // Broadcast Snoop on bus to all other cores
        SnoopResponse snoop = broadcast_snoop(core_id, BusTransactionType::BUS_RD, line_addr);
        if (snoop.shared_asserted) {
            // Another core has it -> Transition to SHARED
            act.new_state = MESIState::SHARED;
        } else {
            // No other core has it -> Transition to EXCLUSIVE
            act.new_state = MESIState::EXCLUSIVE;
        }
        act.latency_cycles = snoop.flushed_data ? 12 : 10; // Peer flush or L2 fetch
        set_state(core_id, line_addr, act.new_state);
        return act;
    }

    // Core Local Write Access: Determines hit/miss and generates BusRdX or BusUpgr
    CoherenceAction handle_cpu_write(size_t core_id, uint32_t addr) {
        CoherenceAction act{};
        uint32_t line_addr = align_to_line(addr);
        MESIState current = get_state(core_id, line_addr);
        act.old_state = current;

        if (current == MESIState::MODIFIED) {
            // Local Hit on M (already have exclusive write permission)
            act.is_hit = true;
            act.new_state = MESIState::MODIFIED;
            act.bus_tx = BusTransactionType::NONE;
            act.latency_cycles = 1;
            return act;
        }

        if (current == MESIState::EXCLUSIVE) {
            // Silent transition from E -> M (no other core has this line)
            act.is_hit = true;
            act.new_state = MESIState::MODIFIED;
            act.bus_tx = BusTransactionType::NONE;
            act.latency_cycles = 1;
            set_state(core_id, line_addr, MESIState::MODIFIED);
            return act;
        }

        if (current == MESIState::SHARED) {
            // Hit in Shared state, but must upgrade to Modified -> Broadcast BusUpgr
            act.is_hit = true;
            act.bus_tx = BusTransactionType::BUS_UPGR;
            SnoopResponse snoop = broadcast_snoop(core_id, BusTransactionType::BUS_UPGR, line_addr);
            act.new_state = MESIState::MODIFIED;
            act.invalidations = snoop.invalidations_caused;
            act.latency_cycles = 2; // Fast upgrade
            set_state(core_id, line_addr, MESIState::MODIFIED);
            return act;
        }

        // Miss in INVALID state -> Broadcast BusRdX
        act.is_hit = false;
        act.bus_tx = BusTransactionType::BUS_RDX;
        SnoopResponse snoop = broadcast_snoop(core_id, BusTransactionType::BUS_RDX, line_addr);
        act.new_state = MESIState::MODIFIED;
        act.invalidations = snoop.invalidations_caused;
        act.latency_cycles = snoop.flushed_data ? 14 : 10;
        set_state(core_id, line_addr, MESIState::MODIFIED);
        return act;
    }

    // Broadcast a bus snoop request to all peer cores
    SnoopResponse broadcast_snoop(size_t initiator_core_id, BusTransactionType tx, uint32_t line_addr) {
        SnoopResponse resp{};
        total_snoop_tx_++;

        for (size_t peer_id = 0; peer_id < num_cores_; ++peer_id) {
            if (peer_id == initiator_core_id) continue;

            // Clear peer's exclusive monitor if set on this line on ANY write
            if ((tx == BusTransactionType::BUS_RDX || tx == BusTransactionType::BUS_UPGR) &&
                monitors_[peer_id].active && monitors_[peer_id].line_addr == line_addr) {
                monitors_[peer_id].active = false;
            }

            MESIState peer_state = get_state(peer_id, line_addr);
            if (peer_state == MESIState::INVALID) continue;

            resp.shared_asserted = true;

            if (tx == BusTransactionType::BUS_RD) {
                // If peer is in Modified, it must flush data and transition to Shared
                if (peer_state == MESIState::MODIFIED) {
                    resp.flushed_data = true;
                    resp.flush_addr = line_addr;
                    set_state(peer_id, line_addr, MESIState::SHARED);
                } else if (peer_state == MESIState::EXCLUSIVE) {
                    set_state(peer_id, line_addr, MESIState::SHARED);
                }
            } else if (tx == BusTransactionType::BUS_RDX || tx == BusTransactionType::BUS_UPGR) {
                // Invalidate peer's line
                if (peer_state == MESIState::MODIFIED) {
                    resp.flushed_data = true;
                    resp.flush_addr = line_addr;
                }
                set_state(peer_id, line_addr, MESIState::INVALID);
                resp.invalidations_caused++;
                total_invalidations_++;
            }
        }
        return resp;
    }

    // Atomic LDREX / STREX support
    void set_exclusive_monitor(size_t core_id, uint32_t addr) {
        if (core_id < num_cores_) {
            monitors_[core_id].active = true;
            monitors_[core_id].line_addr = align_to_line(addr);
        }
    }

    [[nodiscard]] bool check_and_clear_exclusive_monitor(size_t core_id, uint32_t addr) {
        if (core_id >= num_cores_) return false;
        uint32_t line_addr = align_to_line(addr);
        if (monitors_[core_id].active && monitors_[core_id].line_addr == line_addr) {
            monitors_[core_id].active = false;
            return true; // Success (no peer invalidated the line)
        }
        monitors_[core_id].active = false;
        return false; // Failed (invalidated by peer write)
    }

    [[nodiscard]] uint64_t get_total_snoop_tx() const noexcept { return total_snoop_tx_; }
    [[nodiscard]] uint64_t get_total_invalidations() const noexcept { return total_invalidations_; }

private:
    size_t num_cores_{1};
    size_t line_size_{64};
    uint32_t offset_mask_{63};
    std::vector<std::unordered_map<uint32_t, CoreMESILine>> core_lines_;
    std::vector<CoreExclusiveMonitor> monitors_;
    uint64_t access_seq_{0};
    uint64_t total_snoop_tx_{0};
    uint64_t total_invalidations_{0};
};

} // namespace tinyarmsim::uarch
