export module log.buffer;

import std;

namespace details {
    inline constexpr std::size_t LOG_MESSAGE_INLINE_CAPACITY = 128;
    inline constexpr std::size_t PRINTF_FORMAT_INLINE_CAPACITY = 128;
    inline constexpr std::size_t FORMATTED_RECORD_INLINE_CAPACITY = 256;
} // namespace details

export namespace pans::log {
    template <std::size_t INLINE_CAPACITY>
    class InlineBuffer {
    public:
        auto append(char const* data, std::size_t size) -> void {
            if (size == 0) {
                return;
            }

            if (m_overflow.empty() && m_size + size <= INLINE_CAPACITY) {
                std::memcpy(m_inline.data() + m_size, data, size);
                m_size += size;
                return;
            }

            if (m_overflow.empty()) {
                const std::size_t required_capacity = m_size + size;
                m_overflow.reserve(INLINE_CAPACITY * 2);
                m_overflow.insert(m_overflow.end(), m_inline.data(), m_inline.data() + size);
            }
            m_overflow.insert(m_overflow.end(), data, data + size);
            m_size = m_overflow.size();
        }

        auto append(std::string_view data) -> void {
            append(data.data(), data.size());
        }

        auto append(char value) -> void {
            append(&value, 1);
        }

        [[nodiscard]]
        auto data() const noexcept -> char const* {
            return m_overflow.empty() ? m_inline.data() : m_overflow.data();
        }

        [[nodiscard]]
        auto size() const noexcept -> std::size_t {
            return m_size;
        }

        [[nodiscard]]
        auto view() const noexcept -> std::string_view {
            return { data(), size() };
        }

    private:
        std::array<char, INLINE_CAPACITY> m_inline{};
        std::vector<char> m_overflow{};
        std::size_t m_size{};
    };
} // namespace pans::log
