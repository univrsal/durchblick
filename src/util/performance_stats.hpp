#pragma once

#include <cstdint>

namespace PerformanceStats {
void Frame(uint64_t native_pixels, uint64_t rendered_pixels,
    uint64_t render_time_ns);
void SourceCacheHit();
void SourceCacheMiss();
void PlaceholderBatchRebuilt();
}
