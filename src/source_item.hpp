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
#include "item.hpp"
#include "util.h"
#include <QComboBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <obs.hpp>

class CustomWidget : public QWidget {
    Q_OBJECT
public:
    QComboBox* m_combo_box;
    CustomWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        auto* l = new QFormLayout();
        setLayout(l);
        l->setContentsMargins(0, 0, 0, 0);
        m_combo_box = new QComboBox();
        m_combo_box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        l->addRow(T_SOURCE_NAME, m_combo_box);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
};

class SourceItem : public LayoutItem {
    Q_OBJECT
    OBSSource m_src;
    OBSSignal removedSignal;
    QAction* m_toggle_safe_borders;

    bool m_safe_margins_initialized = false;
    gs_vertbuffer_t* m_action_safe_margin {};
    gs_vertbuffer_t* m_graphics_safe_margin {};
    gs_vertbuffer_t* m_four_by_three_safe_margin {};
    gs_vertbuffer_t* m_left_line {};
    gs_vertbuffer_t* m_top_line {};
    gs_vertbuffer_t* m_right_line {};
public slots:
    void toggle_safe_borders(bool);

public:
    static void InitPlaceholder();
    static void OBSSourceRemoved(void* data, calldata_t* params);
    SourceItem(Layout* parent, int x, int y, int w = 1, int h = 1);
    ~SourceItem();

    QWidget* GetConfigWidget() override;
    void LoadConfigFromWidget(QWidget*) override;

    void SetSource(obs_source_t* src);
    virtual void Render(const Config& cfg) override;
    virtual void ContextMenu(QContextMenuEvent* e, QMenu&) override;
};
