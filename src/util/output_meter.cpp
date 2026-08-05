#include "output_meter.hpp"

#include <algorithm>
#include <cmath>
#include <util/platform.h>

OutputMeter::OutputMeter(int x, int y, int height, int channel_width)
    : MixerMeter(nullptr, x, y, height, channel_width, false)
{
    m_channels = 2;

    obs_audio_info audio_info {};
    if (!obs_get_audio_info(&audio_info))
        audio_info.samples_per_sec = 48000;

    audio_convert_info conversion {};
    conversion.format = AUDIO_FORMAT_FLOAT_PLANAR;
    conversion.samples_per_sec = audio_info.samples_per_sec;
    conversion.speakers = SPEAKERS_STEREO;
    conversion.allow_clipping = true;
    obs_add_raw_audio_callback(0, &conversion, AudioCallback, this);
}

OutputMeter::~OutputMeter()
{
    obs_remove_raw_audio_callback(0, AudioCallback, this);
}

void OutputMeter::Render(float cell_scale, float source_scale_x,
    float source_scale_y)
{
    m_last_render_time.store(os_gettime_ns(), std::memory_order_relaxed);
    MixerMeter::Render(cell_scale, source_scale_x, source_scale_y);
}

void OutputMeter::AudioCallback(void* data, size_t, audio_data* audio)
{
    if (!audio || !audio->frames)
        return;

    auto* meter = static_cast<OutputMeter*>(data);
    const uint64_t last_render = meter->m_last_render_time.load(
        std::memory_order_relaxed);
    if (!last_render || os_gettime_ns() - last_render > 500000000ULL)
        return;

    float magnitude[MAX_AUDIO_CHANNELS];
    float peak[MAX_AUDIO_CHANNELS];
    float input_peak[MAX_AUDIO_CHANNELS];
    std::fill_n(magnitude, MAX_AUDIO_CHANNELS, -M_INFINITE);
    std::fill_n(peak, MAX_AUDIO_CHANNELS, -M_INFINITE);
    std::fill_n(input_peak, MAX_AUDIO_CHANNELS, -M_INFINITE);

    for (size_t channel = 0; channel < 2; ++channel) {
        const auto* samples = reinterpret_cast<const float*>(audio->data[channel]);
        if (!samples)
            continue;

        double sum_squares = 0.0;
        float max_sample = 0.0f;
        for (uint32_t frame = 0; frame < audio->frames; ++frame) {
            const float sample = std::abs(samples[frame]);
            max_sample = std::max(max_sample, sample);
            sum_squares += double(sample) * sample;
        }

        const float rms = float(std::sqrt(sum_squares / audio->frames));
        magnitude[channel] = rms > 0.0f ? 20.0f * std::log10(rms) : -M_INFINITE;
        peak[channel] = max_sample > 0.0f ? 20.0f * std::log10(max_sample) : -M_INFINITE;
        input_peak[channel] = peak[channel];
    }

    meter->Update(magnitude, peak, input_peak);
}
