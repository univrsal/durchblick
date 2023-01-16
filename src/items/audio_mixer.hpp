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

#include "../util/mixer_renderer.hpp"
#include "../util/util.h"
#include "item.hpp"
#include <QApplication>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <obs.hpp>

class MixerItemWidget : public QWidget {
    Q_OBJECT
public:
    QSpinBox* m_channel_width;
    MixerItemWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        auto* form = new QFormLayout(this);
        auto* l = new QHBoxLayout();
        l->setContentsMargins(0, 0, 0, 0);
        m_channel_width = new QSpinBox(this);
        form->addRow(T_LABEL_CHANNEL_WIDTH, m_channel_width);
        form->setContentsMargins(0, 0, 0, 0);

        setLayout(form);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
};

class AudioMixerItem : public LayoutItem {
    Q_OBJECT
    bool m_program { false };
    std::unique_ptr<AudioMixerRenderer> m_mixer {};

public:
    AudioMixerItem(Layout* parent, int x, int y, int w = 1, int h = 1)
        : LayoutItem(parent, x, y, w, h)
    {
        m_mixer = std::make_unique<AudioMixerRenderer>(this);
    }

    ~AudioMixerItem() = default;
    QWidget* GetConfigWidget() override;
    void LoadConfigFromWidget(QWidget*) override;
    void Render(DurchblickItemConfig const& cfg) override;

    void WriteToJson(QJsonObject& Obj) override;
    void ReadFromJson(QJsonObject const& Obj) override;

    void ContextMenu(QMenu&) override { }
    virtual void Update(DurchblickItemConfig const& cfg) override;

    void MouseEvent(MouseData const& e, DurchblickItemConfig const& cfg) override;
};
