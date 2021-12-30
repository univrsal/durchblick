/*************************************************************************
 * This file is part of input-overlay
 * git.vrsal.xyz/alex/durchblick
 * Copyright 2021 univrsal <uni@vrsal.xyz>.
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

#include "util.h"
#include <QMouseEvent>
#include <memory>
#include <obs-module.h>
#include <vector>

inline void startRegion(int vX, int vY, int vCX, int vCY, float oL,
    float oR, float oT, float oB)
{
    gs_projection_push();
    gs_viewport_push();
    gs_set_viewport(vX, vY, vCX, vCY);
    gs_ortho(oL, oR, oT, oB, -100.0f, 100.0f);
}

inline void endRegion()
{
    gs_viewport_pop();
    gs_projection_pop();
}

inline void GetScaleAndCenterPos(int baseCX, int baseCY, int windowCX,
    int windowCY, int& x, int& y,
    float& scale)
{
    double windowAspect, baseAspect;
    int newCX, newCY;

    windowAspect = double(windowCX) / double(windowCY);
    baseAspect = double(baseCX) / double(baseCY);

    if (windowAspect > baseAspect) {
        scale = float(windowCY) / float(baseCY);
        newCX = int(double(windowCY) * baseAspect);
        newCY = windowCY;
    } else {
        scale = float(windowCX) / float(baseCX);
        newCX = windowCX;
        newCY = int(float(windowCX) / baseAspect);
    }

    x = windowCX / 2 - newCX / 2;
    y = windowCY / 2 - newCY / 2;
}

// Default item, renders black rectangle
class LayoutItem {
    bool m_mouse_over { false };

public:
    struct {
        int col {}, row {}, w {}, h {};
    } m_cell;
    int m_left {}, m_top {}, m_right {}, m_bottom {},                 // position inside the entire display
        m_rel_left {}, m_rel_top {}, m_rel_right {}, m_rel_bottom {}, // position relative to multiview origin
        m_width {}, m_height {}, m_inner_width {}, m_inner_height {};

    struct Config {
        int x {}, y {}; // Origin of multiview
        float scale {};
        float border = 4;
        float border2 = border * 2;
        float cell_width, cell_height;
    };

    struct MouseData {
        int x, y;
        Qt::KeyboardModifiers modifiers;
        Qt::MouseButtons buttons;
    };

    LayoutItem(int x, int y, int w = 1, int h = 1)
    {
        m_cell = { x, y, w, h };
    }

    virtual void Render(Config const& cfg)
    {
        if (m_mouse_over) {
            DrawBox(0, 0, cfg.cell_width * m_width, cfg.cell_height * m_height, 0xFF000000);
        }
        DrawBox(cfg.border, cfg.border, m_inner_width, m_inner_height, 0xFF000000);
    }

    virtual void MouseEvent(MouseData const& e, Config const& cfg)
    {
        if (m_cell.col == 0 && m_cell.row == 0)
            binfo("%i %i // %i %i %i %i", e.x, e.y, m_left, m_top, m_right, m_bottom);
        m_mouse_over = e.x >= m_rel_left && e.x < m_rel_right && e.y >= m_rel_top && e.y < m_rel_bottom;
    }

    virtual void Update(Config const& cfg)
    {
        m_rel_left = cfg.cell_width * m_cell.col;
        m_left = cfg.x + m_rel_left;
        m_rel_right = cfg.cell_width * (m_cell.col + 1);
        m_right = cfg.x + m_rel_right;
        m_rel_top = cfg.cell_height * m_cell.row;
        m_top = cfg.y + m_rel_top;
        m_rel_bottom = cfg.cell_height * (m_cell.row + 1);
        m_bottom = cfg.y + m_rel_bottom;
        m_width = cfg.cell_width * m_cell.w;
        m_height = cfg.cell_height * m_cell.h;
        m_inner_width = m_width - cfg.border2;
        m_inner_height = m_height - cfg.border2;
    }

    static void DrawBox(float cx, float cy, uint32_t colorVal)
    {
        gs_effect_t* solid = obs_get_base_effect(OBS_EFFECT_SOLID);
        gs_eparam_t* color = gs_effect_get_param_by_name(solid, "color");

        gs_effect_set_color(color, colorVal);
        while (gs_effect_loop(solid, "Solid"))
            gs_draw_sprite(nullptr, 0, (uint32_t)cx, (uint32_t)cy);
    }

    static void DrawBox(float tx, float ty, float cx, float cy, uint32_t color)
    {
        gs_matrix_push();
        gs_matrix_translate3f(tx, ty, 0.0f);
        DrawBox(cx, cy, color);
        gs_matrix_pop();
    };
};

class Layout {
    int m_size;
    std::vector<std::unique_ptr<LayoutItem>> m_layout_items;
    LayoutItem::Config m_cfg;

public:
    Layout(int size = 4)
        : m_size(size)
    {
        for (int x = 0; x < size; x++) {
            for (int y = 0; y < size; y++) {
                m_layout_items.emplace_back(new LayoutItem(x, y));
            }
        }
    }

    void MouseMoved(QMouseEvent* e)
    {
        LayoutItem::MouseData d {
            .x = int((e->x() - m_cfg.x) / m_cfg.scale),
            .y = int((e->y() - m_cfg.y) / m_cfg.scale),
            .modifiers = e->modifiers(),
            .buttons = e->buttons()
        };
        for (auto& Item : m_layout_items)
            Item->MouseEvent(d, m_cfg);
    }

    void Render(int target_cx, int target_cy, uint32_t cx, uint32_t cy)
    {
        // Define the whole usable region for the multiview
        startRegion(m_cfg.x, m_cfg.y, target_cx * m_cfg.scale, target_cy * m_cfg.scale, 0.0f, target_cx,
            0.0f, target_cy);
        LayoutItem::DrawBox(target_cx, target_cy, 0xFFD0D0D0);

        auto setRegion = [&](float bx, float by, float cx, float cy) {
            float vX = int(m_cfg.x + bx * m_cfg.scale);
            float vY = int(m_cfg.y + by * m_cfg.scale);
            float vCX = int(cx * m_cfg.scale);
            float vCY = int(cy * m_cfg.scale);

            float oL = bx;
            float oT = by;
            float oR = (bx + cx);
            float oB = (by + cy);
            startRegion(vX, vY, vCX, vCY, oL, oR, oT, oB);
        };
        for (auto& Item : m_layout_items) {
            // Change region to item dimensions
            setRegion(Item->m_rel_left, Item->m_rel_top, Item->m_width, Item->m_height);
            gs_matrix_push();
            gs_matrix_translate3f(Item->m_rel_left, Item->m_rel_top, 0);
            Item->Render(m_cfg);
            gs_matrix_pop();
            endRegion();
        }
        endRegion();
    }

    void Resize(int target_cx, int target_cy, int cx, int cy)
    {
        // We calculate most layout values only on resize here
        GetScaleAndCenterPos(target_cx, target_cy, cx, cy, m_cfg.x, m_cfg.y, m_cfg.scale);
        m_cfg.cell_width = float(target_cx) / m_size;
        m_cfg.cell_height = float(target_cy) / m_size;
        for (auto& Item : m_layout_items)
            Item->Update(m_cfg);
    }
};
