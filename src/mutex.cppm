module;

#if defined(_MSC_VER)
    #include <intrin.h>
#elif defined(__x86_64__) || defined(__i386__)
    #include <immintrin.h>
#endif

#if defined(__linux__) && defined(__GLIBC__)
    #include <pthread.h>
#endif

#include <cassert>

export module mutex;

import std;

namespace pans::details {

    inline constexpr std::size_t cache_line_size = 64;

    inline auto cpu_relax() noexcept -> void {
#if defined(_MSC_VER)
    #if defined(_M_ARM64) || defined(_M_ARM)
        __yield();
    #else
        _mm_pause();
    #endif
#elif defined(__x86_64__) || defined(__i386__)
        _mm_pause();
#elif defined(__aarch64__) || defined(__arm__)
        asm volatile("yield" ::: "memory");
#endif
    }

} // namespace pans::details

export namespace pans {

    class SpinLock {
    public:
        SpinLock(SpinLock const&)            = delete;
        SpinLock& operator=(SpinLock const&) = delete;
        SpinLock(SpinLock&&)                 = delete;
        SpinLock& operator=(SpinLock&&)      = delete;

#if defined(__linux__) && defined(__GLIBC__)

    public:
        SpinLock() {
            if (int rc = pthread_spin_init(&m_mutex, PTHREAD_PROCESS_PRIVATE); rc != 0) {
                throw std::system_error(rc, std::generic_category(), "pthread_spin_init");
            }
        }

        ~SpinLock() noexcept {
            [[maybe_unused]] int rc = pthread_spin_destroy(&m_mutex);
            assert(rc == 0 && "destroying a spin lock that is still locked");
        }

        auto lock() noexcept -> void {
            (void)pthread_spin_lock(&m_mutex);
        }

        auto unlock() noexcept -> void {
            (void)pthread_spin_unlock(&m_mutex);
        }

        [[nodiscard]]
        auto try_lock() noexcept -> bool {
            return pthread_spin_trylock(&m_mutex) == 0;
        }

    private:
        alignas(details::cache_line_size) pthread_spinlock_t m_mutex{};

#else

    public:
        SpinLock()  = default;
        ~SpinLock() = default;

        auto lock() noexcept -> void {
            while (m_mutex.test_and_set(std::memory_order_acquire)) {
                while (m_mutex.test(std::memory_order_relaxed)) {
                    details::cpu_relax();
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

#endif
    };

} // namespace pans
