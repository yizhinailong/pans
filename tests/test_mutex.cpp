#include <cstdio>

import std;
import mutex;

using u64               = std::uint64_t;

constexpr int RUNS      = 7;
u64           g_Counter = 0;
u64           g_target  = 0;

class AtomicFlagSpinLock {
public:
    AtomicFlagSpinLock() noexcept                            = default;
    ~AtomicFlagSpinLock() noexcept                           = default;

    AtomicFlagSpinLock(const AtomicFlagSpinLock&)            = delete;
    AtomicFlagSpinLock& operator=(const AtomicFlagSpinLock&) = delete;

    auto lock() noexcept -> void {
        while (m_flag.test_and_set(std::memory_order_acquire)) {
            while (m_flag.test(std::memory_order_relaxed)) {
                std::this_thread::yield();
            }
        }
    }

    [[nodiscard]] auto try_lock() noexcept -> bool {
        return !m_flag.test_and_set(std::memory_order_acquire);
    }

    auto unlock() noexcept -> void {
        m_flag.clear(std::memory_order_release);
    }

private:
    std::atomic_flag m_flag{};
};

class AtomicWaitLock {
public:
    using Lock                                       = std::lock_guard<AtomicWaitLock>;

    AtomicWaitLock() noexcept                        = default;
    ~AtomicWaitLock() noexcept                       = default;

    AtomicWaitLock(const AtomicWaitLock&)            = delete;
    AtomicWaitLock& operator=(const AtomicWaitLock&) = delete;

    auto lock() noexcept -> void {
        bool expected = false;
        while (!m_mutex.compare_exchange_weak(expected, true, std::memory_order_acquire, std::memory_order_relaxed)) {
            std::atomic_wait_explicit(&m_mutex, true, std::memory_order_relaxed);
            expected = false;
        }
    }

    [[nodiscard]] auto try_lock() noexcept -> bool {
        bool expected = false;
        return m_mutex.compare_exchange_strong(expected, true, std::memory_order_acquire, std::memory_order_relaxed);
    }

    auto unlock() noexcept -> void {
        m_mutex.store(false, std::memory_order_release);
        std::atomic_notify_one(&m_mutex);
    }

private:
    std::atomic<bool> m_mutex{ false };
};

auto OperationsForThread(std::size_t thread_index, std::size_t thread_count) -> u64 {
    const u64 base      = g_target / thread_count;
    const u64 remainder = g_target % thread_count;
    return base + (thread_index < remainder ? 1 : 0);
}

template <typename Operation>
auto BenchmarkThreads(std::size_t thread_count, Operation&& operation) -> std::chrono::nanoseconds {
    std::latch ready(static_cast<std::ptrdiff_t>(thread_count));
    std::latch start_gate(1);
    std::latch finished(static_cast<std::ptrdiff_t>(thread_count));

    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    for (std::size_t thread_index = 0; thread_index < thread_count; ++thread_index) {
        threads.emplace_back([&, thread_index]() {
            ready.count_down();
            start_gate.wait();
            operation(thread_index, OperationsForThread(thread_index, thread_count));
            finished.count_down();
        });
    }

    ready.wait();
    const auto begin = std::chrono::steady_clock::now();
    start_gate.count_down();
    finished.wait();
    const auto end = std::chrono::steady_clock::now();

    for (std::thread& thread : threads) {
        thread.join();
    }

    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin);
}

template <typename Mutex>
auto BenchmarkMutex(std::size_t thread_count) -> std::chrono::nanoseconds {
    Mutex mutex;
    g_Counter          = 0;

    const auto elapsed = BenchmarkThreads(thread_count, [&mutex](std::size_t, u64 operations) {
        for (u64 index = 0; index < operations; ++index) {
            std::lock_guard<Mutex> lock(mutex);
            ++g_Counter;
        }
    });

    if (g_Counter != g_target) {
        throw std::runtime_error("mutex correctness check failed");
    }

    return elapsed;
}

template <typename Benchmark>
auto Run(Benchmark&& benchmark) -> double {
    long double total_nanoseconds = 0.0L;
    for (int run = 0; run < RUNS; ++run) {
        total_nanoseconds += static_cast<long double>(benchmark().count());
    }
    return static_cast<double>(total_nanoseconds / RUNS);
}

auto PrintResult(std::string_view name, double total_nanoseconds) -> void {
    const double nanoseconds_per_operation = total_nanoseconds / static_cast<double>(g_target);
    std::println("{:<30}{:.2f} ns \ttotal, {:.2f} ns/op", name, total_nanoseconds, nanoseconds_per_operation);
}

auto main(int argc, char** argv) -> int {
    if (argc > 2) {
        std::println(stderr, "Usage: {} [count]", argv[0]);
        return 1;
    }
    if (argc == 2) {
        try {
            g_target = std::stoull(argv[1]);
        } catch (const std::exception& error) {
            std::println(stderr, "invalid count '{}': {}", argv[1], error.what());
            return 1;
        }
    }
    if (g_target == 0) {
        g_target = 100000;
    }

    std::println("operations: {}, average of {} runs", g_target, RUNS);
    std::println();

    const std::vector<std::size_t> thread_counts = { 1, 2, 4, 8, 10 };
    for (const std::size_t thread_count : thread_counts) {
        std::println("threads: {}", thread_count);
        PrintResult("pthread_spinlock::SpinLock", Run([&]() { return BenchmarkMutex<pans::SpinLock>(thread_count); }));
        PrintResult("atomic_flag::SpinLock", Run([&]() { return BenchmarkMutex<AtomicFlagSpinLock>(thread_count); }));
        PrintResult("atomic_wait", Run([&]() { return BenchmarkMutex<AtomicWaitLock>(thread_count); }));
        PrintResult("std::mutex", Run([&]() { return BenchmarkMutex<std::mutex>(thread_count); }));
        std::println();
    }

    return 0;
}
