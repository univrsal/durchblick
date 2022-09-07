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
#include "../util/volume_meter.hpp"
#include "item.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <memory>
#include <mutex>
#include <obs.hpp>

class SourceItemWidget : public QWidget {
    Q_OBJECT
public:
    QComboBox* m_combo_box;
    QDoubleSpinBox* m_font_size;
    QCheckBox* m_show_volume_meter;
    QSpinBox* m_channel_width;
    QDoubleSpinBox* m_volume_meter_height;
    SourceItemWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        auto* l = new QFormLayout(this);
        setLayout(l);
        l->setContentsMargins(0, 0, 0, 0);
        m_combo_box = new QComboBox(this);
        m_combo_box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        m_font_size = new QDoubleSpinBox(this);
        m_font_size->setMinimum(1);
        m_font_size->setValue(100);
        m_font_size->setMaximum(500);
        m_font_size->setSuffix("%");
        m_font_size->setDecimals(0);
        m_show_volume_meter = new QCheckBox(T_SOURCE_ITEM_VOLUME, this);
        m_channel_width = new QSpinBox(this);
        m_volume_meter_height = new QDoubleSpinBox(this);
        m_volume_meter_height->setMinimum(10);
        m_volume_meter_height->setMaximum(100);
        m_volume_meter_height->setValue(50);
        m_volume_meter_height->setSuffix("%");
        m_channel_width->setMinimum(2);
        m_channel_width->setMaximum(32);
        l->addRow(T_SOURCE_NAME, m_combo_box);
        l->addRow(T_FONT_SIZE, m_font_size);
        l->addRow("", m_show_volume_meter);
        l->addRow(T_CHANNEL_WIDTH, m_channel_width);
        l->addRow(T_VOLUME_METER_HEIGHT, m_volume_meter_height);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
};

class SourceItem : public LayoutItem {
    Q_OBJECT
protected:
    bool m_dragging_volume {};
    int m_drag_start_x {}, m_drag_start_y {};
    OBSSource m_src;
    OBSSourceAutoRelease m_label;
    OBSSignal removedSignal;
    QAction* m_toggle_safe_borders;
    QAction* m_toggle_label;
    QAction* m_toggle_volume;
    std::unique_ptr<VolumeMeter> m_vol_meter {};
    float m_font_scale { 1 };
    float m_volume_meter_height { .5 };
    int m_volume_meter_x { 10 }, m_volume_meter_y { 10 };
    int m_channel_width { 2 };
    void RenderSafeMargins(int w, int h);
    static OBSSource CreateLabel(const char* name, size_t h, float scale = 1.0);
    vec2 m_scale {};
public slots:

    void VolumeToggled(bool);

public:
    static void Init();
    static void Deinit();
    static void OBSSourceRemoved(void* data, calldata_t* params);
    SourceItem(Layout* parent, int x, int y, int w = 1, int h = 1);
    ~SourceItem();

    QWidget* GetConfigWidget() override;
    void LoadConfigFromWidget(QWidget*) override;

    void SetSource(obs_source_t* src);

    void SetLabel(bool b)
    {
        m_toggle_label->setChecked(b);
    }

    void SetVolume(bool b)
    {
        m_toggle_volume->setChecked(b);
    }

    void SetSafeBorders(bool b)
    {
        m_toggle_safe_borders->setChecked(b);
    }

    OBSSource GetSource() { return m_src; }

    virtual void ReadFromJson(QJsonObject const& Obj) override;
    virtual void WriteToJson(QJsonObject& Obj) override;
    virtual void Render(DurchblickItemConfig const& cfg) override;
    virtual void ContextMenu(QMenu&) override;
    virtual void MouseEvent(MouseData const& e, DurchblickItemConfig const& cfg) override;

    virtual bool EnableVolumeMeter() const { return true; }
};
