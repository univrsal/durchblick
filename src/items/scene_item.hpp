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

#include "../util/util.h"
#include "source_item.hpp"
#include <QComboBox>
#include <QFormLayout>
#include <QRadioButton>
#include <QVBoxLayout>
#include <obs.hpp>
#if _WIN32
#    include <obs-frontend-api.h>
#else
#    include <obs/obs-frontend-api.h>
#endif

class SceneItemWidget : public QWidget {
    Q_OBJECT
public:
    QComboBox* m_combo_box;
    QRadioButton *m_icon, *m_border, *m_none;
    SceneItemWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        auto* l = new QFormLayout();
        setLayout(l);
        l->setContentsMargins(0, 0, 0, 0);
        m_combo_box = new QComboBox();
        m_combo_box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        l->addRow(T_SCENE_NAME, m_combo_box);

        auto* radio_buttons = new QVBoxLayout(this);
        m_border = new QRadioButton(T_BORDER_INDICATOR, this);
        m_icon = new QRadioButton(T_ICON_INDICATOR, this);
        m_none = new QRadioButton(T_NO_INDICATOR, this);
        radio_buttons->addWidget(m_border);
        radio_buttons->addWidget(m_icon);
        radio_buttons->addWidget(m_none);
        m_border->setChecked(true);
        radio_buttons->setContentsMargins(0, 0, 0, 0);
        l->addRow("", radio_buttons);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
};

class SceneItem : public SourceItem {
    Q_OBJECT

    enum Indicator {
        NONE,
        BORDER,
        ICON,
    } m_indicator_type
        = Indicator::BORDER;

    uint32_t GetIndicatorColor()
    {
        OBSSourceAutoRelease previewSrc = obs_frontend_get_current_preview_scene();
        OBSSourceAutoRelease programSrc = obs_frontend_get_current_scene();
        bool studioMode = obs_frontend_preview_program_mode_active();

        if (m_src == programSrc)
            return COLOR_PROGRAM_INDICATOR;
        else if (m_src == previewSrc)
            return studioMode ? COLOR_PREVIEW_INDICATOR : COLOR_PROGRAM_INDICATOR;
        return 0;
    }

public:
    SceneItem(Layout* parent, int x, int y, int w = 1, int h = 1)
        : SourceItem(parent, x, y, w, h)
    {
    }

    ~SceneItem()
    {
    }

    QWidget* GetConfigWidget() override;
    void LoadConfigFromWidget(QWidget*) override;
    void MouseEvent(MouseData const& e, DurchblickItemConfig const& cfg) override;
    void Render(DurchblickItemConfig const& cfg) override;
    uint32_t GetFillColor() override;
    void ReadFromJson(QJsonObject const& Obj) override;
    void WriteToJson(QJsonObject& Obj) override;
};