/*************************************************************************
 * This file is part of durchblick
 * git.vrsal.xyz/alex/durchblick
 * Copyright 2022 univrsal <uni@vrsal.xyz>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *************************************************************************/
#include "volume_meter.hpp"
#include "util.h"
#include <QTimer>
#include <obs.hpp>
#include <util/platform.h>
#include <util/util.hpp>

#define CLAMP(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))
#define FADER_PRECISION 4096.0

// Size of the audio indicator in pixels
#define INDICATOR_THICKNESS 3
#define CLIP_FLASH_DURATION_MS 1000

static void volume_meter(void* data,
    const float magnitude[MAX_AUDIO_CHANNELS],
    const float peak[MAX_AUDIO_CHANNELS],
    const float inputPeak[MAX_AUDIO_CHANNELS])
{
    static_cast<VolumeMeter*>(data)->update(magnitude, peak, inputPeak);
}

void VolumeMeter::draw_rectangle(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t c)
{
    if (!(w > 0 && h > 0))
        return;
    gs_effect_t* solid = obs_get_base_effect(OBS_EFFECT_SOLID);
    gs_eparam_t* color = gs_effect_get_param_by_name(solid, "color");

    gs_matrix_push();
    gs_matrix_translate3f(x, y, 0);
    gs_effect_set_color(color, c);
    while (gs_effect_loop(solid, "Solid"))
        gs_draw_sprite(nullptr, 0, (uint32_t)w, (uint32_t)h);
    gs_matrix_pop();
}

VolumeMeter::VolumeMeter(OBSSource src, int x, int y, int height, int channel_width)
    : m_x(x)
    , m_y(y)
    , m_height(height)
    , m_channel_width(channel_width)
{
    set_type(OBS_FADER_LOG);
    set_source(src);
    m_minimum_level = -60.0;            // -60 dB
    m_warning_level = -20.0;            // -20 dB
    m_error_level = -9.0;               //  -9 dB
    m_clip_level = -0.5;                //  -0.5 dB
    m_minimum_input_level = -50.0;      // -50 dB
    m_peak_decay_rate = 11.76;          //  20 dB / 1.7 sec
    m_magnitude_integration_time = 0.3; //  99% in 300 ms
    m_peak_hold_duration = 20.0;        //  20 seconds
    m_input_peak_hold_duration = 1.0;   //  1 second
    m_channel_thickness = 3;            // Bar thickness in pixels
                                        // channels = (int)audio_output_get_channels(obs_get_audio());

    m_background_nominal_color = ARGB32(0xff, 0x26, 0x7f, 0x26); // Dark green
    m_background_warning_color = ARGB32(0xff, 0x7f, 0x7f, 0x26); // Dark yellow
    m_background_error_color = ARGB32(0xff, 0x7f, 0x26, 0x26);   // Dark red
    m_foreground_nominal_color = ARGB32(0xff, 0x4c, 0xff, 0x4c); // Bright green
    m_foreground_warning_color = ARGB32(0xff, 0xff, 0xff, 0x4c); // Bright yellow
    m_foreground_error_color = ARGB32(0xff, 0xff, 0x4c, 0x4c);   // Bright red

    m_background_nominal_color_disabled = ARGB32(0xff, 90, 90, 90);
    m_background_warning_color_disabled = ARGB32(0xff, 117, 117, 117);
    m_background_error_color_disabled = ARGB32(0xff, 65, 65, 65);
    m_foreground_nominal_color_disabled = ARGB32(0xff, 163, 163, 163);
    m_foreground_warning_color_disabled = ARGB32(0xff, 217, 217, 217);
    m_foreground_error_color_disabled = ARGB32(0xff, 113, 113, 113);

    m_clip_color = ARGB32(0xff, 0xff, 0xff, 0xff);       // Bright white
    m_magniteude_color = ARGB32(0xff, 0x00, 0x00, 0x00); // Black
    m_major_tick_color = ARGB32(0xff, 0xff, 0xff, 0xff); // Black
    m_minor_tick_color = ARGB32(0xff, 0xcc, 0xcc, 0xcc); // Black
}

VolumeMeter::~VolumeMeter()
{
    obs_volmeter_remove_callback(m_meter, volume_meter, this);
    obs_volmeter_destroy(m_meter);
}

void VolumeMeter::set_type(obs_fader_type t)
{
    obs_volmeter_remove_callback(m_meter, volume_meter, this);
    obs_volmeter_destroy(m_meter);
    m_meter = obs_volmeter_create(t);
    obs_volmeter_add_callback(m_meter, volume_meter, this);
    m_channels = obs_volmeter_get_nr_channels(m_meter);
}

void VolumeMeter::update(const float magnitude[], const float peak[], const float inputPeak[])
{
    uint64_t ts = os_gettime_ns();
    QMutexLocker locker(&m_data_mutex);

    m_current_last_update_time = ts;

    if (m_clipping) {
        if ((ts - m_clip_begin_time) * 0.000000001 > CLIP_FLASH_DURATION_MS) {
            m_clipping = false;
        }
    }

    for (int channelNr = 0; channelNr < MAX_AUDIO_CHANNELS; channelNr++) {
        m_current_magnitude[channelNr] = magnitude[channelNr];
        m_current_peak[channelNr] = peak[channelNr];
        m_current_input_peak[channelNr] = inputPeak[channelNr];
    }

    // In case there are more updates then redraws we must make sure
    // that the ballistics of peak and hold are recalculated.
    locker.unlock();
    calculateBallistics(ts);
}

static void on_source_muted(void* data, calldata_t* calldata)
{
    VolumeMeter* meter = static_cast<VolumeMeter*>(data);
    meter->set_muted(calldata_bool(calldata, "muted"));
}

void VolumeMeter::set_source(OBSSource src)
{
    m_source = src;
    if (m_meter) {

        int currentNrAudioChannels = obs_volmeter_get_nr_channels(m_meter);

        m_muted = obs_source_muted(src);
        signal_handler_connect(obs_source_get_signal_handler(src), "mute", on_source_muted, this);
        if (!currentNrAudioChannels) {
            struct obs_audio_info oai;
            obs_get_audio_info(&oai);
            currentNrAudioChannels = (oai.speakers == SPEAKERS_MONO) ? 1 : 2;
        }
        m_channels = currentNrAudioChannels;

        obs_volmeter_detach_source(m_meter);
        obs_volmeter_attach_source(m_meter, m_source);
    }
}

inline void
VolumeMeter::calculateBallisticsForChannel(int channelNr, uint64_t ts,
    qreal timeSinceLastRedraw)
{
    if (m_current_peak[channelNr] >= m_display_peak[channelNr] || isnan(m_display_peak[channelNr])) {
        // Attack of peak is immediate.
        m_display_peak[channelNr] = m_current_peak[channelNr];
    } else {
        // Decay of peak is 40 dB / 1.7 seconds for Fast Profile
        // 20 dB / 1.7 seconds for Medium Profile (Type I PPM)
        // 24 dB / 2.8 seconds for Slow Profile (Type II PPM)
        float decay = float(m_peak_decay_rate * timeSinceLastRedraw);
        m_display_peak[channelNr] = CLAMP(m_display_peak[channelNr] - decay,
            m_current_peak[channelNr], 0);
    }

    if (m_current_peak[channelNr] >= m_display_peak_hold[channelNr] || !isfinite(m_display_peak_hold[channelNr])) {
        // Attack of peak-hold is immediate, but keep track
        // when it was last updated.
        m_display_peak_hold[channelNr] = m_current_peak[channelNr];
        m_display_peak_hold_last_update_time[channelNr] = ts;
    } else {
        // The peak and hold falls back to peak
        // after 20 seconds.
        qreal timeSinceLastPeak = (uint64_t)(ts - m_display_peak_hold_last_update_time[channelNr]) * 0.000000001;
        if (timeSinceLastPeak > m_peak_hold_duration) {
            m_display_peak_hold[channelNr] = m_current_peak[channelNr];
            m_display_peak_hold_last_update_time[channelNr] = ts;
        }
    }

    if (m_current_input_peak[channelNr] >= m_display_input_peak_hold[channelNr] || !isfinite(m_display_input_peak_hold[channelNr])) {
        // Attack of peak-hold is immediate, but keep track
        // when it was last updated.
        m_display_input_peak_hold[channelNr] = m_current_input_peak[channelNr];
        m_display_input_peak_hold_last_upate_time[channelNr] = ts;
    } else {
        // The peak and hold falls back to peak after 1 second.
        qreal timeSinceLastPeak = (uint64_t)(ts - m_display_input_peak_hold_last_upate_time[channelNr]) * 0.000000001;
        if (timeSinceLastPeak > m_input_peak_hold_duration) {
            m_display_input_peak_hold[channelNr] = m_current_input_peak[channelNr];
            m_display_input_peak_hold_last_upate_time[channelNr] = ts;
        }
    }

    if (!isfinite(m_display_maginuted[channelNr])) {
        // The statements in the else-leg do not work with
        // NaN and infinite displayMagnitude.
        m_display_maginuted[channelNr] = m_current_magnitude[channelNr];
    } else {
        // A VU meter will integrate to the new value to 99% in 300 ms.
        // The calculation here is very simplified and is more accurate
        // with higher frame-rate.
        float attack = float((m_current_magnitude[channelNr] - m_display_maginuted[channelNr]) * (timeSinceLastRedraw / m_magnitude_integration_time) * 0.99);
        m_display_maginuted[channelNr] = CLAMP(m_display_maginuted[channelNr] + attack,
            (float)m_minimum_level, 0);
    }
}

void VolumeMeter::render(float cell_scale)
{
    uint64_t ts = os_gettime_ns();
    qreal timeSinceLastRedraw = (ts - m_last_redraw_time) * 0.000000001;
    calculateBallistics(ts, timeSinceLastRedraw);
    bool idle = detect_idle(ts);

    for (int i = 0; i < m_channels; i++) {
        //        auto magnitude = m_display_maginuted[i];
        auto peak = m_display_peak[i];
        auto peakHold = m_display_peak_hold[i];
        qreal scale = m_height / m_minimum_level;

        QMutexLocker locker(&m_data_mutex);
        int lower_limit = m_y + m_height;
        int upper_limit = m_y;
        //        int magnitude_position = int(lower_limit - (magnitude * scale));
        int peak_position = int(lower_limit - (m_height - (peak * scale)));
        int peakHoldPosition = int(m_y + m_height - (peakHold * scale));
        int nominal_position = int(upper_limit + (m_warning_level * scale));
        int warning_position = int(upper_limit + (m_error_level * scale));

        int nominal_ength = lower_limit - nominal_position;
        int warning_length = nominal_position - warning_position;
        int error_length = warning_position - upper_limit;
        int error_position = 0;

        locker.unlock();
        auto w = m_channel_width / cell_scale;
        auto x = m_x + (w + 2) * i;

        //        if (m_clipping)
        //            peak_position = upper_limit;
        binfo("PEAK: %i", peak_position);
        if (peak_position > lower_limit) { // Peak is below the meter -> no peak visible
            draw_rectangle(x, nominal_position, w, nominal_ength,
                m_muted ? m_background_nominal_color_disabled
                        : m_background_nominal_color);
            draw_rectangle(x, warning_position, w, warning_length,
                m_muted ? m_background_warning_color_disabled
                        : m_background_warning_color);
            draw_rectangle(x, upper_limit, w, error_length,
                m_muted ? m_background_error_color_disabled
                        : m_background_error_color);
        } else if (peak_position > nominal_position) {
            // Nominal (green + background)
            draw_rectangle(x, peak_position, w,
                lower_limit - peak_position,
                m_muted ? m_foreground_nominal_color_disabled
                        : m_foreground_nominal_color);
            draw_rectangle(x, nominal_position, w,
                peak_position - nominal_position,
                m_muted ? m_background_nominal_color_disabled
                        : m_background_nominal_color);

            // Warning (yellow) and error (red)
            draw_rectangle(x, warning_position, w, warning_length,
                m_muted ? m_background_warning_color_disabled
                        : m_background_warning_color);
            draw_rectangle(x, upper_limit, w, error_length,
                m_muted ? m_background_error_color_disabled
                        : m_background_error_color);
        } else if (peak_position > warning_position) {
            draw_rectangle(x, nominal_position, w, nominal_ength,
                m_muted ? m_foreground_nominal_color_disabled
                        : m_foreground_nominal_color);

            // Warning (yellow + background)
            draw_rectangle(x, peak_position, w,
                nominal_position - peak_position,
                m_muted ? m_foreground_warning_color_disabled
                        : m_foreground_warning_color);
            draw_rectangle(x, warning_position, w,
                peak_position - warning_position,
                m_muted ? m_background_warning_color_disabled
                        : m_background_warning_color);

            draw_rectangle(x, upper_limit, w, error_length,
                m_muted ? m_background_error_color_disabled
                        : m_background_error_color);
        } else if (peak_position > error_position) {
            draw_rectangle(x, peak_position, w, warning_position - peak_position,
                m_muted ? m_foreground_error_color_disabled
                        : m_foreground_error_color);
            draw_rectangle(x, upper_limit, w, peak_position - upper_limit,
                m_muted ? m_background_error_color_disabled
                        : m_background_error_color);

            draw_rectangle(x, nominal_position, w, nominal_ength,
                m_muted ? m_foreground_nominal_color_disabled
                        : m_foreground_nominal_color);
            draw_rectangle(x, warning_position, w, warning_length,
                m_muted ? m_foreground_warning_color_disabled
                        : m_foreground_warning_color);
        } else {
            if (!m_clipping) {
                m_clip_begin_time = os_gettime_ns();
                m_clipping = true;
            }

            //            int end = error_length + warning_length + nominal_ength;
            //            draw_rectangle(x, minimumPosition, w, end,
            //                muted ? foregroundErrorColorDisabled
            //                      : foregroundErrorColor);
        }

        if (peakHoldPosition - 3 < lower_limit)
            ; // Peak-hold below minimum, no drawing.
        else if (peakHoldPosition < warning_position)
            draw_rectangle(x, upper_limit - peakHoldPosition - 3, w, 3,
                m_muted ? m_foreground_nominal_color_disabled
                        : m_foreground_nominal_color);
        else if (peakHoldPosition < error_position)
            draw_rectangle(x, upper_limit - peakHoldPosition - 3, w, 3,
                m_muted ? m_foreground_warning_color_disabled
                        : m_foreground_warning_color);
        else
            draw_rectangle(x, upper_limit - peakHoldPosition - 3, w, 3,
                m_muted ? m_foreground_error_color
                        : m_foreground_error_color);

        //        if (magnitudePosition - 3 >= minimumPosition)
        //            draw_rectangle(x, magnitudePosition + 5, w, 3,
        //                magnitudeColor);

        if (idle)
            continue;
        //        uint32_t color;
        //        if (peakHold < m_minimum_input_level)
        //            color = m_background_nominal_color;
        //        else if (peakHold < m_warning_level)
        //            color = m_foreground_nominal_color;
        //        else if (peakHold < m_error_level)
        //            color = m_foreground_warning_color;
        //        else if (peakHold <= m_clip_level)
        //            color = m_foreground_error_color;
        //        else
        //            color = m_clip_color;

        //        draw_rectangle(i * (meterThickness + 1), m_y - 5, meterThickness, INDICATOR_THICKNESS, color);
    }
}

inline void VolumeMeter::calculateBallistics(uint64_t ts,
    qreal timeSinceLastRedraw)
{
    QMutexLocker locker(&m_data_mutex);

    for (int channelNr = 0; channelNr < MAX_AUDIO_CHANNELS; channelNr++)
        calculateBallisticsForChannel(channelNr, ts,
            timeSinceLastRedraw);
}
