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
    minimumLevel = -60.0;           // -60 dB
    warningLevel = -20.0;           // -20 dB
    errorLevel = -9.0;              //  -9 dB
    clipLevel = -0.5;               //  -0.5 dB
    minimumInputLevel = -50.0;      // -50 dB
    peakDecayRate = 11.76;          //  20 dB / 1.7 sec
    magnitudeIntegrationTime = 0.3; //  99% in 300 ms
    peakHoldDuration = 20.0;        //  20 seconds
    inputPeakHoldDuration = 1.0;    //  1 second
    meterThickness = 3;             // Bar thickness in pixels
                                    // channels = (int)audio_output_get_channels(obs_get_audio());

    backgroundNominalColor = ARGB32(0xff, 0x26, 0x7f, 0x26); // Dark green
    backgroundWarningColor = ARGB32(0xff, 0x7f, 0x7f, 0x26); // Dark yellow
    backgroundErrorColor = ARGB32(0xff, 0x7f, 0x26, 0x26);   // Dark red
    foregroundNominalColor = ARGB32(0xff, 0x4c, 0xff, 0x4c); // Bright green
    foregroundWarningColor = ARGB32(0xff, 0xff, 0xff, 0x4c); // Bright yellow
    foregroundErrorColor = ARGB32(0xff, 0xff, 0x4c, 0x4c);   // Bright red

    backgroundNominalColorDisabled = ARGB32(0xff, 90, 90, 90);
    backgroundWarningColorDisabled = ARGB32(0xff, 117, 117, 117);
    backgroundErrorColorDisabled = ARGB32(0xff, 65, 65, 65);
    foregroundNominalColorDisabled = ARGB32(0xff, 163, 163, 163);
    foregroundWarningColorDisabled = ARGB32(0xff, 217, 217, 217);
    foregroundErrorColorDisabled = ARGB32(0xff, 113, 113, 113);

    clipColor = ARGB32(0xff, 0xff, 0xff, 0xff);      // Bright white
    magnitudeColor = ARGB32(0xff, 0x00, 0x00, 0x00); // Black
    majorTickColor = ARGB32(0xff, 0xff, 0xff, 0xff); // Black
    minorTickColor = ARGB32(0xff, 0xcc, 0xcc, 0xcc); // Black
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
    QMutexLocker locker(&dataMutex);

    currentLastUpdateTime = ts;
    for (int channelNr = 0; channelNr < MAX_AUDIO_CHANNELS; channelNr++) {
        currentMagnitude[channelNr] = magnitude[channelNr];
        currentPeak[channelNr] = peak[channelNr];
        currentInputPeak[channelNr] = inputPeak[channelNr];
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

        muted = obs_source_muted(src);
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
    if (currentPeak[channelNr] >= displayPeak[channelNr] || isnan(displayPeak[channelNr])) {
        // Attack of peak is immediate.
        displayPeak[channelNr] = currentPeak[channelNr];
    } else {
        // Decay of peak is 40 dB / 1.7 seconds for Fast Profile
        // 20 dB / 1.7 seconds for Medium Profile (Type I PPM)
        // 24 dB / 2.8 seconds for Slow Profile (Type II PPM)
        float decay = float(peakDecayRate * timeSinceLastRedraw);
        displayPeak[channelNr] = CLAMP(displayPeak[channelNr] - decay,
            currentPeak[channelNr], 0);
    }

    if (currentPeak[channelNr] >= displayPeakHold[channelNr] || !isfinite(displayPeakHold[channelNr])) {
        // Attack of peak-hold is immediate, but keep track
        // when it was last updated.
        displayPeakHold[channelNr] = currentPeak[channelNr];
        displayPeakHoldLastUpdateTime[channelNr] = ts;
    } else {
        // The peak and hold falls back to peak
        // after 20 seconds.
        qreal timeSinceLastPeak = (uint64_t)(ts - displayPeakHoldLastUpdateTime[channelNr]) * 0.000000001;
        if (timeSinceLastPeak > peakHoldDuration) {
            displayPeakHold[channelNr] = currentPeak[channelNr];
            displayPeakHoldLastUpdateTime[channelNr] = ts;
        }
    }

    if (currentInputPeak[channelNr] >= displayInputPeakHold[channelNr] || !isfinite(displayInputPeakHold[channelNr])) {
        // Attack of peak-hold is immediate, but keep track
        // when it was last updated.
        displayInputPeakHold[channelNr] = currentInputPeak[channelNr];
        displayInputPeakHoldLastUpdateTime[channelNr] = ts;
    } else {
        // The peak and hold falls back to peak after 1 second.
        qreal timeSinceLastPeak = (uint64_t)(ts - displayInputPeakHoldLastUpdateTime[channelNr]) * 0.000000001;
        if (timeSinceLastPeak > inputPeakHoldDuration) {
            displayInputPeakHold[channelNr] = currentInputPeak[channelNr];
            displayInputPeakHoldLastUpdateTime[channelNr] = ts;
        }
    }

    if (!isfinite(displayMagnitude[channelNr])) {
        // The statements in the else-leg do not work with
        // NaN and infinite displayMagnitude.
        displayMagnitude[channelNr] = currentMagnitude[channelNr];
    } else {
        // A VU meter will integrate to the new value to 99% in 300 ms.
        // The calculation here is very simplified and is more accurate
        // with higher frame-rate.
        float attack = float((currentMagnitude[channelNr] - displayMagnitude[channelNr]) * (timeSinceLastRedraw / magnitudeIntegrationTime) * 0.99);
        displayMagnitude[channelNr] = CLAMP(displayMagnitude[channelNr] + attack,
            (float)minimumLevel, 0);
    }
}

void VolumeMeter::render(float cell_scale)
{
    uint64_t ts = os_gettime_ns();
    qreal timeSinceLastRedraw = (ts - lastRedrawTime) * 0.000000001;
    calculateBallistics(ts, timeSinceLastRedraw);
    bool idle = detect_idle(ts);

    for (int i = 0; i < m_channels; i++) {
        auto magnitude = displayMagnitude[i];
        auto peak = displayPeak[i];
        auto peakHold = displayPeakHold[i];
        qreal scale = m_height / minimumLevel;

        QMutexLocker locker(&dataMutex);
        int minimumPosition = m_y;
        int maximumPosition = m_y + m_height;
        int magnitudePosition = int(m_y + m_height - (magnitude * scale));
        int peakPosition = int(m_y + m_height - (peak * scale));
        int peakHoldPosition = int(m_y + m_height - (peakHold * scale));
        int warningPosition = int(m_y + m_height - (warningLevel * scale));
        int errorPosition = int(m_y + m_height - (errorLevel * scale));

        int nominalLength = warningPosition - minimumPosition;
        int warningLength = errorPosition - warningPosition;
        int errorLength = maximumPosition - errorPosition;

        locker.unlock();
        auto w = m_channel_width / cell_scale;
        auto x = m_x + (w * 1.5) * i;

        if (m_clipping)
            peakPosition = maximumPosition;

        if (peakPosition < minimumPosition) {
            draw_rectangle(x, maximumPosition - nominalLength, w, nominalLength,
                muted ? backgroundNominalColorDisabled
                      : backgroundNominalColor);
            draw_rectangle(x, maximumPosition - warningLength - nominalLength, w, warningLength,
                muted ? backgroundWarningColorDisabled
                      : backgroundWarningColor);
            draw_rectangle(x, maximumPosition - warningLength - nominalLength - errorLength, w, errorLength,
                muted ? backgroundErrorColorDisabled
                      : backgroundErrorColor);
        } else if (peakPosition < warningPosition) {
            // Nominal (green + background)
            draw_rectangle(x, maximumPosition - peakPosition, w,
                peakPosition,
                muted ? foregroundNominalColorDisabled
                      : foregroundNominalColor);
            draw_rectangle(x, maximumPosition - warningPosition, w,
                warningPosition - peakPosition,
                muted ? backgroundNominalColorDisabled
                      : backgroundNominalColor);

            // Warning (yellow) and error (red)
            draw_rectangle(x, maximumPosition - warningLength - nominalLength, w, warningLength,
                muted ? backgroundWarningColorDisabled
                      : backgroundWarningColor);
            draw_rectangle(x, maximumPosition - warningLength - nominalLength - errorLength, w, errorLength,
                muted ? backgroundErrorColorDisabled
                      : backgroundErrorColor);
        } else if (peakPosition < errorPosition) {
            draw_rectangle(x, maximumPosition - nominalLength, w, nominalLength,
                muted ? foregroundNominalColorDisabled
                      : foregroundNominalColor);

            // Warning (yellow + background)
            draw_rectangle(x, maximumPosition - nominalLength - (peakPosition - warningPosition), w,
                peakPosition - warningPosition,
                muted ? foregroundWarningColorDisabled
                      : foregroundWarningColor);
            draw_rectangle(x, maximumPosition - nominalLength - warningLength, w,
                errorPosition - peakPosition,
                muted ? backgroundWarningColorDisabled
                      : backgroundWarningColor);

            draw_rectangle(x, maximumPosition - warningLength - nominalLength - errorLength, w, errorLength,
                muted ? backgroundErrorColorDisabled
                      : backgroundErrorColor);
        } else if (peakPosition < maximumPosition) {
            draw_rectangle(x, maximumPosition - nominalLength, w, nominalLength,
                muted ? foregroundNominalColorDisabled
                      : foregroundNominalColor);
            draw_rectangle(x, maximumPosition - warningLength - nominalLength, w, warningLength,
                muted ? foregroundWarningColorDisabled
                      : foregroundWarningColor);

            draw_rectangle(x, minimumPosition, w,
                maximumPosition - peakPosition,
                muted ? backgroundErrorColorDisabled
                      : backgroundErrorColor);
            draw_rectangle(x, minimumPosition + (maximumPosition - peakPosition), w,
                peakPosition - errorPosition,
                muted ? foregroundErrorColorDisabled
                      : foregroundErrorColor);
        } else {
            if (!m_clipping) {
                //                QTimer::singleShot(CLIP_FLASH_DURATION_MS, this,
                //                    SLOT(ClipEnding()));
                m_clipping = true;
            }

            int end = errorLength + warningLength + nominalLength;
            //            draw_rectangle(x, minimumPosition, w, end,
            //                muted ? foregroundErrorColorDisabled
            //                      : foregroundErrorColor);
        }

        //        if (peakHoldPosition - 3 < minimumPosition)
        //            ; // Peak-hold below minimum, no drawing.
        //        else if (peakHoldPosition < warningPosition)
        //            draw_rectangle(x, peakHoldPosition - 3, w, 3,
        //                muted ? foregroundNominalColorDisabled
        //                      : foregroundNominalColor);
        //        else if (peakHoldPosition < errorPosition)
        //            draw_rectangle(x, peakHoldPosition - 3, w, 3,
        //                muted ? foregroundWarningColorDisabled
        //                      : foregroundWarningColor);
        //        else
        //            draw_rectangle(x, peakHoldPosition - 3, w, 3,
        //                muted ? foregroundErrorColorDisabled
        //                      : foregroundErrorColor);

        //        if (magnitudePosition - 3 >= minimumPosition)
        //            draw_rectangle(x, magnitudePosition + 5, w, 3,
        //                magnitudeColor);

        if (idle)
            continue;
        uint32_t color;
        if (peakHold < minimumInputLevel)
            color = backgroundNominalColor;
        else if (peakHold < warningLevel)
            color = foregroundNominalColor;
        else if (peakHold < errorLevel)
            color = foregroundWarningColor;
        else if (peakHold <= clipLevel)
            color = foregroundErrorColor;
        else
            color = clipColor;

        //        draw_rectangle(i * (meterThickness + 1), m_y - 5, meterThickness, INDICATOR_THICKNESS, color);
    }
}

inline void VolumeMeter::calculateBallistics(uint64_t ts,
    qreal timeSinceLastRedraw)
{
    QMutexLocker locker(&dataMutex);

    for (int channelNr = 0; channelNr < MAX_AUDIO_CHANNELS; channelNr++)
        calculateBallisticsForChannel(channelNr, ts,
            timeSinceLastRedraw);
}
