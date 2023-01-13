/**
 ** This file is part of the durchblick project.
 ** Copyright 2022 univrsal <uni@vrsal.xyz>.
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **/

#pragma once
#include "../items/item.hpp"
#include "callbacks.h"
#include "volume_meter.hpp"
#include <vector>

class AudioMixerRenderer;

class MixerSlider : public MixerMeter {
    OBSSource m_label {};
    obs_fader_t* m_fader {};
    bool m_dragging_volume { false }, m_lmb_down { false };
    float m_db {}, m_fade { 1 };
    AudioMixerRenderer* m_parent {};
    int m_mute_width;
    int m_mute_height;

protected:
    void OnSourceNameChanged() override;

    void OnSourceVolumeChanged() override
    {
        SetDb(obs_fader_get_db(m_fader));
    }

public:
    MixerSlider(AudioMixerRenderer*, OBSSource, int x = 10, int y = 10, int height = 100, int channel_width = 3);
    ~MixerSlider();

    virtual void Render(float cell_scale, float source_scale_x, float source_scale_y) override;

    void SetHeight(int h) { m_height = h; }
    void SetY(int y) { m_y = y; }

    float GetSliderPosition()
    {
        return 1 - m_fade;
    }

    void SetSource(OBSSource) override;

    void SetType(obs_fader_type) override;

    void SetDb(float db)
    {
        m_db = db;
        m_fade = obs_fader_get_deflection(m_fader);
    }

    bool MouseOverMuteArea(int x, int y)
    {
        return x >= m_x - 2 && x <= m_x + m_mute_width + 2 && y >= m_y + m_height - m_mute_height - 2 && y <= m_y + m_height + 2;
    }

    bool MouseOverSlider(int x, int y)
    {
        const int slider_width = m_channel_width * 1.5;
        const int left = m_x + GetWidth() + 2;
        const int right = m_x + GetWidth() + 20 + slider_width;
        return x >= left && x <= right && y >= m_y && y <= m_y + m_height;
    }

    void MouseEvent(const LayoutItem::MouseData& e, const DurchblickItemConfig& cfg, uint32_t mx, uint32_t my);
};

class AudioMixerItem;

class AudioMixerRenderer {
    std::vector<std::unique_ptr<MixerSlider>> m_sliders;
    int m_height {}, m_y {}, m_channel_width {};
    AudioMixerItem* m_parent {};
    bool m_update_queued { false };

    void RefreshSliderSizeAndPos();

public:
    AudioMixerRenderer(AudioMixerItem* parent, int height = 100, int channel_width = 3);
    ~AudioMixerRenderer() = default;

    void UpdateSources();
    void QueueSourceUpdate() { m_update_queued = true; }
    void Render(float cell_scale, float source_scale_x, float source_scale_y);
    void Update(DurchblickItemConfig const& cfg);

    void MouseEvent(const LayoutItem::MouseData& e, const DurchblickItemConfig& cfg);
    void SetChannelWidth(int w)
    {
        m_channel_width = w;
        for (auto& slider : m_sliders)
            slider->SetChannelWidth(w);
    }
};
