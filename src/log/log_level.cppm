export module log;

import std;

export namespace pans::log::level {

    enum class Level : std::size_t {
        TRACE,
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        FATAL,
        OFF,
    };

    // Level 的枚举值连续，可直接作为下标，ToString 为 O(1)。
    inline constexpr std::array<std::string_view, 7> kNames{
        "TRACE",
        "DEBUG",
        "INFO",
        "WARNING",
        "ERROR",
        "FATAL",
        "OFF",
    };

    static_assert(std::to_underlying(Level::OFF) + 1 == kNames.size());

    [[nodiscard]]
    constexpr auto ToString(Level level) noexcept -> std::string_view {
        const auto index = std::to_underlying(level);
        return index < kNames.size() ? kNames[index] : std::string_view{ "UNKNOWN" };
    }

    [[nodiscard]]
    constexpr auto FromString(std::string_view name) noexcept -> Level {
        for (std::size_t index = 0; index < kNames.size(); ++index) {
            if (std::ranges::equal(name, kNames[index], [](char lhs, char rhs) constexpr noexcept {
                    const auto upper = (lhs >= 'a' && lhs <= 'z') ? static_cast<char>(lhs - ('a' - 'A')) : lhs;
                    return upper == rhs;
                })) {
                return static_cast<Level>(index);
            }
        }
        return Level::OFF;
    }

} // namespace pans::log::level
