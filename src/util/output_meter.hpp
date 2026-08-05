#pragma once

#include "volume_meter.hpp"
#include <atomic>

class OutputMeter final : public MixerMeter {
    std::atomic<uint64_t> m_last_render_time {};
    static void AudioCallback(void* data, size_t mix_idx,
        audio_data* audio);

public:
    OutputMeter(int x = 10, int y = 10, int height = 100,
        int channel_width = 4);
    ~OutputMeter() override;
    void Render(float cell_scale, float source_scale_x,
        float source_scale_y) override;
};
