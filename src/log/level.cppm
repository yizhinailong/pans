export module log.level;

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
        constexpr auto EqualIgnoreCase = [](std::string_view lhs, std::string_view rhs) constexpr noexcept {
            return std::ranges::equal(lhs, rhs, [](char lhs_char, char rhs_char) constexpr noexcept {
                const auto upper =
                    (lhs_char >= 'a' && lhs_char <= 'z') ?
                    static_cast<char>(lhs_char - ('a' - 'A')) :
                    lhs_char;
                return upper == rhs_char;
            });
        };

        for (std::size_t index = 0; index < kNames.size(); ++index) {
            if (EqualIgnoreCase(name, kNames[index])) {
                return static_cast<Level>(index);
            }
        }
        return Level::OFF;
    }

} // namespace pans::log::level
