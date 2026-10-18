export module mutex;

import std;

namespace pans::details {

    inline constexpr std::size_t cache_line_size = 64;

} // namespace pans::details

export namespace pans {

    class SpinLock {
    public:
        SpinLock()                           = default;
        ~SpinLock()                          = default;

        SpinLock(SpinLock const&)            = delete;
        SpinLock& operator=(SpinLock const&) = delete;
        SpinLock(SpinLock&&)                 = delete;
        SpinLock& operator=(SpinLock&&)      = delete;

        auto lock() noexcept -> void {
            while (m_mutex.test_and_set(std::memory_order_acquire)) {
                while (m_mutex.test(std::memory_order_relaxed)) {
                    std::this_thread::yield();
                }
            }
        }

        auto unlock() noexcept -> void {
            m_mutex.clear(std::memory_order_release);
        }

        [[nodiscard]]
        auto try_lock() noexcept -> bool {
            return !m_mutex.test_and_set(std::memory_order_acquire);
        }

    private:
        alignas(details::cache_line_size) std::atomic_flag m_mutex{};
    };

} // namespace pans
