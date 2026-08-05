#include "performance_stats.hpp"
#include "util.h"

#include <atomic>
#include <util/platform.h>

namespace {
std::atomic<uint64_t> frames {};
std::atomic<uint64_t> native_pixels {};
std::atomic<uint64_t> rendered_pixels {};
std::atomic<uint64_t> render_time_ns {};
std::atomic<uint64_t> cache_hits {};
std::atomic<uint64_t> cache_misses {};
std::atomic<uint64_t> placeholder_rebuilds {};
std::atomic<uint64_t> last_report_time {};
}

namespace PerformanceStats {
void Frame(uint64_t native, uint64_t rendered, uint64_t elapsed)
{
    frames.fetch_add(1, std::memory_order_relaxed);
    native_pixels.fetch_add(native, std::memory_order_relaxed);
    rendered_pixels.fetch_add(rendered, std::memory_order_relaxed);
    render_time_ns.fetch_add(elapsed, std::memory_order_relaxed);

    const uint64_t now = os_gettime_ns();
    uint64_t previous = last_report_time.load(std::memory_order_relaxed);
    if (!previous) {
        last_report_time.compare_exchange_strong(previous, now,
            std::memory_order_relaxed);
        return;
    }
    if (now - previous < 30000000000ULL)
        return;
    if (!last_report_time.compare_exchange_strong(previous, now,
            std::memory_order_relaxed))
        return;

    const uint64_t frame_count = frames.exchange(0, std::memory_order_relaxed);
    if (!frame_count)
        return;
    const uint64_t native_count = native_pixels.exchange(
        0, std::memory_order_relaxed);
    const uint64_t rendered_count = rendered_pixels.exchange(
        0, std::memory_order_relaxed);
    const uint64_t elapsed_ns = render_time_ns.exchange(
        0, std::memory_order_relaxed);
    const uint64_t hits = cache_hits.exchange(0, std::memory_order_relaxed);
    const uint64_t misses = cache_misses.exchange(0, std::memory_order_relaxed);
    const uint64_t rebuilds = placeholder_rebuilds.exchange(
        0, std::memory_order_relaxed);
    const double pixel_reduction = native_count
        ? 100.0 * (1.0 - double(rendered_count) / native_count)
        : 0.0;

    binfo("Performance: %.2f ms/frame, %.1f%% fewer multiview pixels, "
          "source cache %llu/%llu hits, %llu grid batch rebuilds",
        double(elapsed_ns) / frame_count / 1000000.0, pixel_reduction,
        static_cast<unsigned long long>(hits),
        static_cast<unsigned long long>(hits + misses),
        static_cast<unsigned long long>(rebuilds));
}

void SourceCacheHit()
{
    cache_hits.fetch_add(1, std::memory_order_relaxed);
}

void SourceCacheMiss()
{
    cache_misses.fetch_add(1, std::memory_order_relaxed);
}

void PlaceholderBatchRebuilt()
{
    placeholder_rebuilds.fetch_add(1, std::memory_order_relaxed);
}
}
