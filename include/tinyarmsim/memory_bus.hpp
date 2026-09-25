#pragma once

#include <cstdint>
#include <vector>
#include <cstring>
#include "tinyarmsim/faults.hpp"

namespace tinyarmsim {

class MemoryBus {
public:
    explicit MemoryBus(size_t size_bytes = 64 * 1024 * 1024)
        : size_(size_bytes), memory_(size_bytes, 0) {}

    [[nodiscard]] size_t size() const noexcept { return size_; }

    [[nodiscard]] uint8_t read8(uint32_t address) const {
        check_bounds(address, 1);
        return memory_[address];
    }

    void write8(uint32_t address, uint8_t value) {
        check_bounds(address, 1);
        memory_[address] = value;
    }

    [[nodiscard]] uint16_t read16(uint32_t address) const {
        check_alignment(address, 2);
        check_bounds(address, 2);
        uint16_t val;
        std::memcpy(&val, &memory_[address], sizeof(val));
        return val;
    }

    void write16(uint32_t address, uint16_t value) {
        check_alignment(address, 2);
        check_bounds(address, 2);
        std::memcpy(&memory_[address], &value, sizeof(value));
    }

    [[nodiscard]] uint32_t read32(uint32_t address) const {
        check_alignment(address, 4);
        check_bounds(address, 4);
        uint32_t val;
        std::memcpy(&val, &memory_[address], sizeof(val));
        return val;
    }

    void write32(uint32_t address, uint32_t value) {
        check_alignment(address, 4);
        check_bounds(address, 4);
        std::memcpy(&memory_[address], &value, sizeof(value));
    }

    // Direct buffer pointer access for fast bulk loading (e.g. ELF segments)
    [[nodiscard]] uint8_t* get_raw_ptr(uint32_t address, size_t length) {
        check_bounds(address, length);
        return &memory_[address];
    }

    [[nodiscard]] const uint8_t* get_raw_ptr(uint32_t address, size_t length) const {
        check_bounds(address, length);
        return &memory_[address];
    }

private:
    void check_bounds(uint32_t address, size_t length) const {
        if (static_cast<uint64_t>(address) + length > size_) {
            throw MemoryFaultException(FaultType::MemoryOutOfBounds, address, "Memory access out of bounds");
        }
    }

    void check_alignment(uint32_t address, uint32_t align_bytes) const {
        if ((address % align_bytes) != 0) {
            throw MemoryFaultException(FaultType::MemoryUnaligned, address, "Unaligned memory access");
        }
    }

    size_t size_;
    std::vector<uint8_t> memory_;
};

} // namespace tinyarmsim
