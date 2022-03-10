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

#include "layout.hpp"

Layout::Layout(QWidget* parent, int size)
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

void Layout::MouseMoved(QMouseEvent* e)
{
    LayoutItem::MouseData d {
        .x = int((e->x() - m_cfg.x) / m_cfg.scale),
        .y = int((e->y() - m_cfg.y) / m_cfg.scale),
        .modifiers = e->modifiers(),
        .buttons = e->buttons(),
        .type = e->type()
    };

    LayoutItem::Cell pos;
    bool anything_hovered = false;
    for (auto& Item : m_layout_items) {
        Item->MouseEvent(d, m_cfg);
        if (Item->Hovered()) {
            pos = Item->m_cell;
            anything_hovered = true;
        }
    }
    m_hovered_cell = pos;
    if (anything_hovered && e->buttons() & Qt::RightButton) {
        // Dragging
        m_selection_end = pos;
        m_dragging = true;
    } else {
        m_dragging = false;
    }

    if (!m_dragging) {
        m_selection_end.clear();
        m_selection_start.clear();
    }
}

void Layout::MousePressed(QMouseEvent* e)
{
    LayoutItem::MouseData d {
        .x = int((e->x() - m_cfg.x) / m_cfg.scale),
        .y = int((e->y() - m_cfg.y) / m_cfg.scale),
        .modifiers = e->modifiers(),
        .buttons = e->buttons(),
        .type = e->type()
    };
    for (auto& Item : m_layout_items)
        Item->MouseEvent(d, m_cfg);
    if (e->button() == Qt::RightButton) {
        m_selection_start = m_hovered_cell;
    } else {
        m_selection_end.clear();
        m_selection_start.clear();
        m_dragging = false;
    }
}

void Layout::MouseReleased(QMouseEvent* e)
{
    LayoutItem::MouseData d {
        .x = int((e->x() - m_cfg.x) / m_cfg.scale),
        .y = int((e->y() - m_cfg.y) / m_cfg.scale),
        .modifiers = e->modifiers(),
        .buttons = e->buttons(),
        .type = e->type()
    };
    for (auto& Item : m_layout_items)
        Item->MouseEvent(d, m_cfg);
    m_dragging = false;
}

void Layout::HandleContextMenu(QContextMenuEvent*)
{
    // Keep drawing the selection if it wasn't reset
    if (!m_selection_end.empty())
        m_dragging = true;
    for (auto& Item : m_layout_items) {
        if (Item->Hovered()) {
            QMenu m(T_MENU_OPTION, m_parent_widget);
            m.addAction(m_new_widget_action);
            m.addSeparator();
            Item->ContextMenu(m);
            m.exec(QCursor::pos());
            break;
        }
    }
}

void Layout::FreeSpace(const LayoutItem::Cell& c)
{
    auto it = std::remove_if(m_layout_items.begin(), m_layout_items.end(), [c](std::unique_ptr<LayoutItem> const& item) {
        auto result = !item || c.Overlaps(item->m_cell) || item->m_cell.Overlaps(c);
        return result;
    });
    m_layout_items.erase(it, m_layout_items.end());
}

void Layout::AddWidget(const Registry::ItemRegistry::Entry& entry, QWidget* custom_widget)
{
    std::lock_guard<std::mutex> lock(m_layout_mutex);
    LayoutItem::Cell target = m_hovered_cell;

    if (m_dragging) {
        int tx, ty, cx, cy;
        GetSelection(tx, ty, cx, cy);
        target.col = tx;
        target.row = ty;
        target.w = cx;
        target.h = cy;
    }

    FreeSpace(target);
    auto* Item = entry.construct(this, target.col, target.row, target.w, target.h, entry.priv);
    Item->LoadConfigFromWidget(custom_widget);
    Item->Update(m_cfg);
    m_layout_items.emplace_back(Item);
}

void Layout::SetRegion(float bx, float by, float cx, float cy)
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

void Layout::Render(int target_cx, int target_cy, uint32_t cx, uint32_t cy)
{
    // Define the whole usable region for the multiview
    startRegion(m_cfg.x, m_cfg.y, target_cx * m_cfg.scale, target_cy * m_cfg.scale, 0.0f, target_cx,
        0.0f, target_cy);
    LayoutItem::DrawBox(target_cx, target_cy, 0xFFD0D0D0);

    if (m_dragging) {
        int tx, ty, cx, cy;
        GetSelection(tx, ty, cx, cy);
        LayoutItem::DrawBox(tx * m_cfg.cell_width, ty * m_cfg.cell_height, cx * m_cfg.cell_width, cy * m_cfg.cell_height, 0xFF009999);
    }

    m_layout_mutex.lock();
    for (auto& Item : m_layout_items) {
        // Change region to item dimensions
        gs_matrix_push();
        gs_matrix_translate3f(Item->m_rel_left, Item->m_rel_top, 0);

        SetRegion(Item->m_rel_left, Item->m_rel_top, Item->m_width, Item->m_height);
        if (Item->Hovered())
            LayoutItem::DrawBox(0, 0, m_cfg.cell_width * Item->m_width, m_cfg.cell_height * Item->m_height, m_dragging ? 0xFF009999 : 0xFF004400);
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

void Layout::Resize(int target_cx, int target_cy, int cx, int cy)
{
    // We calculate most layout values only on resize here
    GetScaleAndCenterPos(target_cx, target_cy, cx, cy, m_cfg.x, m_cfg.y, m_cfg.scale);
    m_cfg.cell_width = float(target_cx) / m_size;
    m_cfg.cell_height = float(target_cy) / m_size;
    for (auto& Item : m_layout_items)
        Item->Update(m_cfg);
}
