#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace tinyarmsim {

constexpr uint32_t VERSION_MAJOR = 0;
constexpr uint32_t VERSION_MINOR = 1;
constexpr uint32_t VERSION_PATCH = 0;

[[nodiscard]] constexpr std::string_view get_version_string() noexcept {
    return "0.1.0";
}

} // namespace tinyarmsim
