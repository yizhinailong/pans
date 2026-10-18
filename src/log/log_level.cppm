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

    std::unordered_map<std::string_view, Level> table{
        {   "TRACE",   Level::TRACE },
        {   "DEBUG",   Level::DEBUG },
        {    "INFO",    Level::INFO },
        { "WARNING", Level::WARNING },
        {   "ERROR",   Level::ERROR },
        {   "FATAL",   Level::FATAL },
        {     "OFF",     Level::OFF },
    };

    [[nodiscard]]
    auto ToString(Level level) noexcept -> std::string_view {
        for (const auto& [name, code] : table) {
            if (level == code) {
                return name;
            }
        }
        return std::string_view{ "UNKNOW" };
    }

    [[nodiscard]]
    auto FromString(std::string_view level_str) noexcept -> Level {
        std::array<char, 8> normalized{};
        std::ranges::transform(level_str, normalized.begin(), [](unsigned char character) noexcept -> char {
            return static_cast<char>(std::toupper(character));
        });

        const std::string_view name{ normalized.data(), level_str.size() };

        for (const auto& [text, level] : table) {
            if (name == text) {
                return level;
            }
        }
        return Level::OFF;
    }

} // namespace pans::log::level
