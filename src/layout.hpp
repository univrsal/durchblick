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

#include "item.hpp"
#include "new_item_dialog.hpp"
#include "registry.hpp"
#include "util.h"
#include <QMouseEvent>
#include <algorithm>
#include <memory>
#include <mutex>
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

class Layout : public QObject {
    int m_size;
    std::vector<std::unique_ptr<LayoutItem>> m_layout_items;
    LayoutItem::Config m_cfg;
    QWidget* m_parent_widget {};
    QAction* m_new_widget_action {};
    LayoutItem::Cell m_selection {};
    std::mutex m_layout_mutex;
    Q_OBJECT
private slots:

    void ShowSetWidgetDialog()
    {
        NewItemDialog dlg(m_parent_widget, this);
        dlg.exec();
    }

public:
    Layout(QWidget* parent, int size = 4)
        : QObject(parent)
        , m_size(size)
        , m_parent_widget(parent)
    {
        for (int x = 0; x < size; x++) {
            for (int y = 0; y < size; y++) {
                m_layout_items.emplace_back(new LayoutItem(this, x, y));
            }
        }
        m_new_widget_action = new QAction("Set widget", this);
        connect(m_new_widget_action, SIGNAL(triggered()), this, SLOT(ShowSetWidgetDialog()));
    }

    void MouseMoved(QMouseEvent* e)
    {
        LayoutItem::MouseData d {
            .x = int((e->x() - m_cfg.x) / m_cfg.scale),
            .y = int((e->y() - m_cfg.y) / m_cfg.scale),
            .modifiers = e->modifiers(),
            .buttons = e->buttons()
        };

        LayoutItem::Cell pos;
        for (auto& Item : m_layout_items) {
            Item->MouseEvent(d, m_cfg);
            if (Item->Hovered())
                pos = Item->m_cell;
        }
        if (e->buttons() & Qt::LeftButton) {
            // Dragging
        } else {
            m_selection = pos;
        }
    }

    void HandleContextMenu(QContextMenuEvent* e)
    {
        for (auto& Item : m_layout_items) {
            if (Item->Hovered()) {
                QMenu m(T_MENU_OPTION, m_parent_widget);
                m.addAction(m_new_widget_action);
                m.addSeparator();
                Item->ContextMenu(e, m);
                m.exec(QCursor::pos());
                break;
            }
        }
    }

    void FreeSpace(LayoutItem::Cell const& c)
    {
        auto it = std::remove_if(m_layout_items.begin(), m_layout_items.end(), [c](std::unique_ptr<LayoutItem> const& item) {
            return c.Overlaps(item->m_cell);
        });
        m_layout_items.erase(it);
    }

    void AddWidget(Registry::ItemRegistry::Entry const& entry)
    {
        std::lock_guard<std::mutex> lock(m_layout_mutex);
        FreeSpace(m_selection);
        auto* Item = entry.construct(this, m_selection.col, m_selection.row, m_selection.w, m_selection.h, entry.priv);
        Item->Update(m_cfg);
        m_layout_items.emplace_back(Item);
    }

    void SetRegion(float bx, float by, float cx, float cy)
    {
        float vX = int(m_cfg.x + bx * m_cfg.scale);
        float vY = int(m_cfg.y + by * m_cfg.scale);
        float vCX = int(cx * m_cfg.scale);
        float vCY = int(cy * m_cfg.scale);

        float oL = bx;
        float oT = by;
        float oR = (bx + cx);
        float oB = (by + cy);
        startRegion(vX, vY, vCX, vCY, oL, oR, oT, oB);
    }

    void Render(int target_cx, int target_cy, uint32_t cx, uint32_t cy)
    {
        // Define the whole usable region for the multiview
        startRegion(m_cfg.x, m_cfg.y, target_cx * m_cfg.scale, target_cy * m_cfg.scale, 0.0f, target_cx,
            0.0f, target_cy);
        LayoutItem::DrawBox(target_cx, target_cy, 0xFFD0D0D0);

        m_layout_mutex.lock();
        for (auto& Item : m_layout_items) {
            // Change region to item dimensions
            gs_matrix_push();
            gs_matrix_translate3f(Item->m_rel_left, Item->m_rel_top, 0);

            SetRegion(Item->m_rel_left, Item->m_rel_top, Item->m_width, Item->m_height);
            if (Item->Hovered())
                LayoutItem::DrawBox(0, 0, m_cfg.cell_width * Item->m_width, m_cfg.cell_height * Item->m_height, 0xFF004400);
            endRegion();
            gs_matrix_pop();

            gs_matrix_push();
            gs_matrix_translate3f(Item->m_rel_left + m_cfg.border, Item->m_rel_top + m_cfg.border, 0);
            SetRegion(Item->m_rel_left + m_cfg.border, Item->m_rel_top + m_cfg.border, Item->m_inner_width, Item->m_inner_height);
            Item->Render(m_cfg);
            endRegion();
            gs_matrix_pop();
        }
        m_layout_mutex.unlock();
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
