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
#pragma once
#include <QColor>
#include <QMutex>
#include <QtGlobal>
#include <cstdint>
#include <obs-module.h>
#include <obs.hpp>

class VolumeMeter {
    bool muted = false;
    uint64_t lastRedrawTime = 0;
    uint64_t currentLastUpdateTime = 0;
    int m_channels = 0;
    bool m_clipping = false;
    OBSSource m_source;
    obs_volmeter_t* m_meter {};

    int m_x, m_y, m_height, m_channel_width;

    float currentMagnitude[MAX_AUDIO_CHANNELS];
    float currentPeak[MAX_AUDIO_CHANNELS];
    float currentInputPeak[MAX_AUDIO_CHANNELS];

    int displayNrAudioChannels = 0;
    float displayMagnitude[MAX_AUDIO_CHANNELS];
    float displayPeak[MAX_AUDIO_CHANNELS];
    float displayPeakHold[MAX_AUDIO_CHANNELS];
    uint64_t displayPeakHoldLastUpdateTime[MAX_AUDIO_CHANNELS];
    float displayInputPeakHold[MAX_AUDIO_CHANNELS];
    uint64_t displayInputPeakHoldLastUpdateTime[MAX_AUDIO_CHANNELS];

    int meterThickness;
    qreal minimumLevel;
    qreal warningLevel;
    qreal errorLevel;
    qreal clipLevel;
    qreal minimumInputLevel;
    qreal peakDecayRate;
    qreal magnitudeIntegrationTime;
    qreal peakHoldDuration;
    qreal inputPeakHoldDuration;
    QMutex dataMutex;

    uint32_t backgroundNominalColor;
    uint32_t backgroundWarningColor;
    uint32_t backgroundErrorColor;
    uint32_t foregroundNominalColor;
    uint32_t foregroundWarningColor;
    uint32_t foregroundErrorColor;

    uint32_t backgroundNominalColorDisabled;
    uint32_t backgroundWarningColorDisabled;
    uint32_t backgroundErrorColorDisabled;
    uint32_t foregroundNominalColorDisabled;
    uint32_t foregroundWarningColorDisabled;
    uint32_t foregroundErrorColorDisabled;

    uint32_t clipColor;
    uint32_t magnitudeColor;
    uint32_t majorTickColor;
    uint32_t minorTickColor;

    inline void draw_rectangle(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);

public:
    VolumeMeter(OBSSource, int x = 10, int y = 10, int height = 100, int channel_width = 3);
    ~VolumeMeter();

    bool detect_idle(uint64_t ts)
    {
        double timeSinceLastUpdate = (ts - currentLastUpdateTime) * 0.000000001;
        if (timeSinceLastUpdate > 0.5) {
            reset_levels();
            return true;
        } else {
            return false;
        }
    }

    void set_type(obs_fader_type t);

    void set_muted(bool m) { muted = m; }

    void update(const float magnitude[MAX_AUDIO_CHANNELS],
        const float peak[MAX_AUDIO_CHANNELS],
        const float inputPeak[MAX_AUDIO_CHANNELS]);

    void reset_levels()
    {
        currentLastUpdateTime = 0;
        for (int channelNr = 0; channelNr < MAX_AUDIO_CHANNELS; channelNr++) {
            currentMagnitude[channelNr] = -M_INFINITE;
            currentPeak[channelNr] = -M_INFINITE;
            currentInputPeak[channelNr] = -M_INFINITE;

            displayMagnitude[channelNr] = -M_INFINITE;
            displayPeak[channelNr] = -M_INFINITE;
            displayPeakHold[channelNr] = -M_INFINITE;
            displayPeakHoldLastUpdateTime[channelNr] = 0;
            displayInputPeakHold[channelNr] = -M_INFINITE;
            displayInputPeakHoldLastUpdateTime[channelNr] = 0;
        }
    }

    void set_source(OBSSource);

    void render(float scale);

    void calculateBallistics(uint64_t ts,
        qreal timeSinceLastRedraw = 0.0);

    void calculateBallisticsForChannel(int channelNr, uint64_t ts,
        qreal timeSinceLastRedraw = 0.0);
};
