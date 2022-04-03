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
#include "callbacks.h"
#include "util.h"
#include <QContextMenuEvent>
#include <QJsonObject>
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
    QAction* m_toggle_stretch;

public:
    struct Cell {
    private:
        bool Overlapcheck(Cell const& other) const
        {
            if (other.col >= col && other.col < right() && other.row >= row && other.row < bottom())
                return true;
            if (other.right() > col && other.right() <= right() && other.bottom() > row && other.bottom() <= bottom())
                return true;
            if (other.left() >= col && other.left() < right() && other.bottom() > row && other.bottom() < bottom())
                return true;
            if (other.right() > col && other.right() <= right() && other.top() >= row && other.top() < bottom())
                return true;
            return false;
        }

    public:
        int col {}, row {}, w { 1 }, h { 1 };

        int left() const { return col; }
        int right() const { return col + w; }
        int top() const { return row; }
        int bottom() const { return row + h; }

        bool Overlaps(Cell const& other) const
        {
            return Overlapcheck(other) || other.Overlapcheck(*this);
        }

        bool IsSame(Cell const& other) const
        {
            return col == other.col && row == other.row && w == other.w && h == other.h;
        }
        void clear() { col = row = w = h = 0; }
        bool empty() const { return col == 0 && row == 0 && w == 0 && h == 0; }
    } m_cell, m_hovered_cell; // m_cell is the actual size which can span multiple cells, hovered cell is the exact subregion which the mouse is over

    int m_left {}, m_top {}, m_right {}, m_bottom {},                 // position inside the entire display
        m_rel_left {}, m_rel_top {}, m_rel_right {}, m_rel_bottom {}, // position relative to multiview origin
        m_width {}, m_height {}, m_inner_width {}, m_inner_height {};

    struct MouseData {
        int x, y;
        Qt::KeyboardModifiers modifiers;
        Qt::MouseButtons buttons;
        QEvent::Type type;
        bool double_click {};
        MouseData(int _x, int _y, Qt::KeyboardModifiers const& m, Qt::MouseButtons const& mb, QEvent::Type const& t)
            : x(_x)
            , y(_y)
            , modifiers(m)
            , buttons(mb)
            , type(t)
        {
        }
    };

    LayoutItem(Layout* parent, int x, int y, int w = 1, int h = 1)
        : QObject((QObject*)parent)
        , m_layout(parent)
    {
        m_cell = { x, y, w, h };
        m_toggle_stretch = new QAction(T_WIDGET_STRETCH, this);
        m_toggle_stretch->setCheckable(true);
    }

    virtual ~LayoutItem()
    {
    }

    virtual void WriteToJson(QJsonObject& Obj)
    {
        Obj["col"] = m_cell.col;
        Obj["row"] = m_cell.row;
        Obj["w"] = m_cell.w;
        Obj["h"] = m_cell.h;
        Obj["stretch"] = m_toggle_stretch->isChecked();
        Obj["id"] = metaObject()->className();
    }

    virtual void ReadFromJson(QJsonObject const& Obj)
    {
        m_cell.col = Obj["col"].toInt();
        m_cell.row = Obj["row"].toInt();
        m_cell.w = Obj["w"].toInt();
        m_cell.h = Obj["h"].toInt();
        m_toggle_stretch->setChecked(Obj["stretch"].toBool());
    }

    /// Determines the border of the cell when it is not hovered
    virtual uint32_t GetFillColor() { return 0xFFD0D0D0; }

    virtual QWidget* GetConfigWidget() { return nullptr; }
    virtual void LoadConfigFromWidget(QWidget*) { }

    virtual void ContextMenu(QMenu& m)
    {
        m.addAction(m_toggle_stretch);
    }

    virtual void Render(DurchblickItemConfig const& cfg)
    {
        DrawBox(0, 0, m_inner_width, m_inner_height, 0xFF000000);
    }

    virtual void MouseEvent(MouseData const& e, DurchblickItemConfig const& cfg)
    {
        if (e.type == QEvent::MouseMove)
            m_mouse_over = IsMouseOver(e.x, e.y);
        if (m_mouse_over) {
            int x = e.x / cfg.cell_width;
            int y = e.y / cfg.cell_height;
            m_hovered_cell.col = x;
            m_hovered_cell.row = y;
        }
    }

    bool IsMouseOver(int x, int y)
    {
        m_mouse_over = x >= m_rel_left && x < m_rel_right && y >= m_rel_top && y < m_rel_bottom;
        return m_mouse_over;
    }

    bool Hovered() const { return m_mouse_over; }

    Cell const& HoveredCell()
    {
        return m_hovered_cell;
    }

    virtual void Update(DurchblickItemConfig const& cfg)
    {
        m_rel_left = cfg.cell_width * m_cell.col;
        m_left = cfg.x + m_rel_left;
        m_rel_right = cfg.cell_width * (m_cell.col + m_cell.w);
        m_right = cfg.x + m_rel_right;
        m_rel_top = cfg.cell_height * m_cell.row;
        m_top = cfg.y + m_rel_top;
        m_rel_bottom = cfg.cell_height * (m_cell.row + m_cell.h);
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

class PlaceholderItem : public LayoutItem {
    Q_OBJECT
public:
    PlaceholderItem(Layout* parent, int x, int y, int w = 1, int h = 1)
        : LayoutItem(parent, x, y, w, h)
    {
    }
    void ContextMenu(QMenu& m) override { }
};
