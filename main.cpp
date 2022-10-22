#include <iostream>
#include <cmath>
#include <chrono>
#include <vector>
#include <thread>
#include <mutex>

#define ENABLE_DEBUGGING 0

struct Range {
    uint64_t start;
    uint64_t amount;
};

struct Context {
    uint64_t primes_found;
    uint64_t delegated;
    const uint64_t number_limit;
    std::mutex mutex;
#if ENABLE_DEBUGGING
    std::mutex print_mutex;
#endif
};

static struct Context g_ctx = {
    .number_limit = 1000000000
};

static bool isPrime(uint64_t n)
{
    if (n == 1 || n == 0)
        return false;

    for (uint64_t i = 2; i <= sqrt(n); i++)
        if (n % i == 0)
            return false;

    return true;
}

static bool getRange(uint64_t primes_found, std::chrono::duration<double, std::milli> &duration, struct Range &range)
{
    g_ctx.mutex.lock();
    g_ctx.primes_found += primes_found;
    
#if ENABLE_DEBUGGING
    g_ctx.print_mutex.lock();
    std::cout << "Range " << range.start << " to " << range.start + range.amount
              << " Amount: " << range.amount
              << " Primes found: " << primes_found
              << " Duration: " << duration.count()
              << std::endl;
    g_ctx.print_mutex.unlock();
#endif

    const uint64_t remaining = g_ctx.number_limit - g_ctx.delegated;
    if (remaining == 0) {
        g_ctx.mutex.unlock();
        return false;
    }

    constexpr double target_ms = 500;
    constexpr uint64_t min_amount = 100;
    constexpr uint64_t max_amount = 100 * 1000;

    uint64_t next_amount = range.amount * (target_ms / duration.count());
    next_amount = fmax(next_amount, min_amount);
    next_amount = fmin(next_amount, max_amount);
    next_amount = fmin(remaining, next_amount);
    range.start = g_ctx.delegated;
    range.amount = next_amount;
    g_ctx.delegated += next_amount;

    g_ctx.mutex.unlock();
    return true;
}

void primeThread(void* args)
{
    struct Range range = {
            .start = 0,
            .amount = 0
    };
    uint64_t primes_found = 0;
    std::chrono::duration<double, std::milli> duration(0.0);

    while (getRange(primes_found, duration, range)) {
        primes_found = 0;
        auto start = std::chrono::high_resolution_clock::now();

        for (uint64_t num = range.start; num < (range.start + range.amount); num++)
            if (isPrime(num))
                primes_found++;

        auto stop = std::chrono::high_resolution_clock::now();
        duration = (stop - start);
    }
}

int main(void)
{
    std::vector<std::thread> threads;
    auto num_threads = std::thread::hardware_concurrency() - 1;
    threads.reserve(num_threads);

    auto start_time = std::chrono::system_clock::now();

    for (size_t idx = 0; idx < num_threads; idx++)
        threads.emplace_back(primeThread, nullptr);

    for (auto& thread: threads)
        thread.join();

    auto end_time = std::chrono::system_clock::now();

    std::chrono::duration<double, std::milli> duration(end_time - start_time);
    std::cout << "Found " << g_ctx.primes_found << " prime numbers in "
              << duration.count() << "ms." << std::endl;
    return 0;
}
