/*************************************************************************
 * This file is part of input-overlay
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
#include <QContextMenuEvent>
#include <QMenu>
#include <QObject>
#include <obs-module.h>

class Layout;

// Default item, renders black rectangle
class LayoutItem : public QObject {
    Q_OBJECT
protected:
    bool m_mouse_over { false };
    Layout* m_layout {};

public:
    struct Cell {
        int col {}, row {}, w { 1 }, h { 1 };

        int left() const { return col; }
        int right() const { return col + w; }
        int top() const { return row; }
        int bottom() const { return row + h; }
        bool Overlaps(Cell const& other) const
        {
            return top() >= other.top() && bottom() <= other.bottom() && left() >= other.left() && right() <= other.right();
        }
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
        bool m_fill_cell { true }; // strech item to cell (disregards aspect ratio)
    };

    struct MouseData {
        int x, y;
        Qt::KeyboardModifiers modifiers;
        Qt::MouseButtons buttons;
    };

    LayoutItem(Layout* parent, int x, int y, int w = 1, int h = 1)
        : QObject((QObject*)parent)
        , m_layout(parent)
    {
        m_cell = { x, y, w, h };
    }

    virtual ~LayoutItem()
    {
    }

    virtual void ContextMenu(QContextMenuEvent* e, QMenu&) { }

    virtual void Render(Config const& cfg)
    {
        DrawBox(0, 0, m_inner_width, m_inner_height, 0xFF000000);
    }

    virtual void MouseEvent(MouseData const& e, Config const& cfg)
    {
        m_mouse_over = IsMouseOver(e.x, e.y);
    }

    bool IsMouseOver(int x, int y) const
    {
        return x >= m_rel_left && x < m_rel_right && y >= m_rel_top && y < m_rel_bottom;
    }

    bool Hovered() const { return m_mouse_over; }

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
