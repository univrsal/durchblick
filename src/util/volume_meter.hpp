/*************************************************************************
 * This file is part of durchblick
 * git.vrsal.xyz/alex/durchblick
 * Copyright 2023 univrsal <uni@vrsal.xyz>.
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
#pragma once
#include <QColor>
#include <QMutex>
#include <QtGlobal>
#include <cstdint>
#include <obs-module.h>
#include <obs.hpp>

class MixerMeter {
protected:
    bool m_muted = false;
    uint64_t m_clip_begin_time = 0;
    uint64_t m_last_redraw_time = 0;
    uint64_t m_current_last_update_time = 0;
    int m_channels = 0;
    bool m_clipping = false;
    OBSSource m_source;
    obs_volmeter_t* m_meter {};

    int m_x, m_y, m_height, m_channel_width;

    float m_current_magnitude[MAX_AUDIO_CHANNELS];
    float m_current_peak[MAX_AUDIO_CHANNELS];
    float m_current_input_peak[MAX_AUDIO_CHANNELS];

    float m_display_magnitude[MAX_AUDIO_CHANNELS];
    float m_display_peak[MAX_AUDIO_CHANNELS];
    float m_display_peak_hold[MAX_AUDIO_CHANNELS];
    uint64_t m_display_peak_hold_last_update_time[MAX_AUDIO_CHANNELS];
    float m_display_input_peak_hold[MAX_AUDIO_CHANNELS];
    uint64_t m_display_input_peak_hold_last_upate_time[MAX_AUDIO_CHANNELS];

    qreal m_minimum_level;
    qreal m_warning_level;
    qreal m_error_level;
    qreal m_clip_level;
    qreal m_minimum_input_level;
    qreal m_peak_decay_rate;
    qreal m_magnitude_integration_time;
    qreal m_peak_hold_duration;
    qreal m_input_peak_hold_duration;
    QMutex m_data_mutex;

    uint32_t m_background_nominal_color;
    uint32_t m_background_warning_color;
    uint32_t m_background_error_color;
    uint32_t m_foreground_nominal_color;
    uint32_t m_foreground_warning_color;
    uint32_t m_foreground_error_color;

    uint32_t m_background_nominal_color_disabled;
    uint32_t m_background_warning_color_disabled;
    uint32_t m_background_error_color_disabled;
    uint32_t m_foreground_nominal_color_disabled;
    uint32_t m_foreground_warning_color_disabled;
    uint32_t m_foreground_error_color_disabled;

    uint32_t m_clip_color;
    uint32_t m_magnitude_color;
    uint32_t m_major_tick_color;
    uint32_t m_minor_tick_color;

    inline void draw_rectangle(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);

    OBSSignal vol_changed_signal;
    OBSSignal mixersSignal;
    OBSSignal mute_signal;
    OBSSignal deactivateSignal;
    OBSSignal rename_signal;

    virtual void OnSourceNameChanged() { }
    virtual void OnSourceVolumeChanged() { }

public:
    MixerMeter(OBSSource, int x = 10, int y = 10, int height = 100, int channel_width = 3);
    ~MixerMeter();

    bool DetectIdle(uint64_t ts)
    {
        double timeSinceLastUpdate = (ts - m_current_last_update_time) * 0.000000001;
        if (timeSinceLastUpdate > 0.5) {
            ResetLevels();
            return true;
        } else {
            return false;
        }
    }

    virtual void SetType(obs_fader_type t);

    void SetMuted(bool m) { m_muted = m; }

    void Update(const float magnitude[MAX_AUDIO_CHANNELS],
        const float peak[MAX_AUDIO_CHANNELS],
        const float inputPeak[MAX_AUDIO_CHANNELS]);

    void ResetLevels()
    {
        m_current_last_update_time = 0;
        for (int channelNr = 0; channelNr < MAX_AUDIO_CHANNELS; channelNr++) {
            m_current_magnitude[channelNr] = -M_INFINITE;
            m_current_peak[channelNr] = -M_INFINITE;
            m_current_input_peak[channelNr] = -M_INFINITE;

            m_display_magnitude[channelNr] = -M_INFINITE;
            m_display_peak[channelNr] = -M_INFINITE;
            m_display_peak_hold[channelNr] = -M_INFINITE;
            m_display_peak_hold_last_update_time[channelNr] = 0;
            m_display_input_peak_hold[channelNr] = -M_INFINITE;
            m_display_input_peak_hold_last_upate_time[channelNr] = 0;
        }
    }

    int GetX() const { return m_x; }
    int GetY() const { return m_y; }
    int GetHeight() const { return m_height; }
    int GetWidth() const { return (m_channel_width + 2) * m_channels; }
    void SetPos(int x, int y)
    {
        m_x = x;
        m_y = y;
    }

    bool MouseOver(int x, int y)
    {
        return x >= m_x && x <= m_x + GetWidth() && y >= m_y && y <= m_y + m_height;
    }

    virtual void SetSource(OBSSource);

    virtual void Render(float cell_scale, float source_scale_x, float source_scale_y);

    void CalculateBallistics(uint64_t ts,
        qreal timeSinceLastRedraw = 0.0);

    void CalculateBallisticsForChannel(int channelNr, uint64_t ts,
        qreal timeSinceLastRedraw = 0.0);

    void SetChannelWidth(int w)
    {
        m_channel_width = w;
    }
};
