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
#include "config.hpp"
#include "durchblick.hpp"
#include "preview_program_item.hpp"
#include "scene_item.hpp"
#include "util.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#if _WIN32
#    include <obs-frontend-api.h>
#else
#    include <obs/obs-frontend-api.h>
#endif

void Layout::FillEmptyCells()
{
    // Make sure that every cell has a placeholder
    std::vector<LayoutItem::Cell> empty;
    for (int x = 0; x < m_cols; x++) {
        for (int y = 0; y < m_rows; y++) {
            LayoutItem::Cell c;
            c.col = x;
            c.row = y;
            bool isFree = true;
            for (auto const& item : m_layout_items) {
                if (c.Overlaps(item->m_cell)) {
                    isFree = false;
                    break;
                }
            }

            if (isFree)
                empty.emplace_back(c);
        }
    }

    for (auto const& c : empty) {
        auto* Item = new PlaceholderItem(this, c.col, c.row);
        Item->Update(m_cfg);
        m_layout_items.emplace_back(Item);
    }
}

LayoutItem::Cell Layout::GetSelectedArea()
{
    LayoutItem::Cell target = m_hovered_cell;

    if (m_dragging) {
        int tx, ty, cx, cy;
        GetSelection(tx, ty, cx, cy);
        target.col = tx;
        target.row = ty;
        target.w = cx;
        target.h = cy;
    }
    return target;
}

void Layout::ClearSelection()
{
    auto target = GetSelectedArea();
    m_layout_mutex.lock();
    FreeSpace(target);
    FillEmptyCells();
    m_layout_mutex.unlock();
    Config::Save();
}

Layout::Layout(QWidget* parent, int cols, int rows)
    : QObject(parent)
    , m_rows(rows)
    , m_cols(cols)
    , m_parent_widget(parent)
{
}

Layout::~Layout()
{
}

void Layout::MouseMoved(QMouseEvent* e)
{
    LayoutItem::MouseData d(
        int((e->x() - m_cfg.x) / m_cfg.scale),
        int((e->y() - m_cfg.y) / m_cfg.scale),
        e->modifiers(),
        e->buttons(),
        e->type());

    LayoutItem::Cell pos;
    bool anything_hovered = false;
    for (auto& Item : m_layout_items) {
        Item->MouseEvent(d, m_cfg);
        if (Item->Hovered()) {
            pos = Item->m_hovered_cell;
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
    LayoutItem::MouseData d(
        int((e->x() - m_cfg.x) / m_cfg.scale),
        int((e->y() - m_cfg.y) / m_cfg.scale),
        e->modifiers(),
        e->buttons(),
        e->type());
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
    LayoutItem::MouseData d(
        int((e->x() - m_cfg.x) / m_cfg.scale),
        int((e->y() - m_cfg.y) / m_cfg.scale),
        e->modifiers(),
        e->buttons(),
        e->type());
    for (auto& Item : m_layout_items)
        Item->MouseEvent(d, m_cfg);
    m_dragging = false;
}

void Layout::MouseDoubleClicked(QMouseEvent* e)
{
    LayoutItem::MouseData d(
        int((e->x() - m_cfg.x) / m_cfg.scale),
        int((e->y() - m_cfg.y) / m_cfg.scale),
        e->modifiers(),
        e->buttons(),
        e->type());
    d.double_click = true;
    for (auto& Item : m_layout_items)
        Item->MouseEvent(d, m_cfg);
}

void Layout::HandleContextMenu(QMouseEvent*, QMenu& m)
{
    // Keep drawing the selection if it wasn't reset
    if (!m_selection_end.empty())
        m_dragging = true;

    m.addAction(T_MENU_LAYOUT_CONFIG, this, SLOT(ShowLayoutConfigDialog()));
    std::lock_guard<std::mutex> lock(m_layout_mutex);
    for (auto& Item : m_layout_items) {
        if (Item->Hovered()) {
            m.addAction(T_MENU_SET_WIDGET, this, SLOT(ShowSetWidgetDialog()));
            m.addAction(T_MENU_CLEAR_ACTION, this, SLOT(ClearSelection()));
            m.addSeparator();
            Item->ContextMenu(m);
            break;
        }
    }
}

void Layout::FreeSpace(LayoutItem::Cell const& c)
{
    auto it = std::remove_if(m_layout_items.begin(), m_layout_items.end(), [c](std::unique_ptr<LayoutItem> const& item) {
        auto result = !item || c.Overlaps(item->m_cell);
        return result;
    });
    m_layout_items.erase(it, m_layout_items.end());
}

void Layout::AddWidget(Registry::ItemRegistry::Entry const& entry, QWidget* custom_widget)
{
    std::lock_guard<std::mutex> lock(m_layout_mutex);
    auto target = GetSelectedArea();
    FreeSpace(target);
    auto* Item = entry.construct(this, target.col, target.row, target.w, target.h);
    Item->LoadConfigFromWidget(custom_widget);
    Item->Update(m_cfg);
    m_layout_items.emplace_back(Item);
    FillEmptyCells();
    Config::Save();
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
    startRegion(m_cfg.x, m_cfg.y, m_cfg.cx * m_cfg.scale, m_cfg.cy * m_cfg.scale, 0.0f, m_cfg.cx,
        0.0f, m_cfg.cy);
    LayoutItem::DrawBox(m_cfg.cx, m_cfg.cy, 0xFFD0D0D0);

    m_layout_mutex.lock();
    for (auto& Item : m_layout_items) {
        // Change region to item dimensions
        gs_matrix_push();
        gs_matrix_translate3f(Item->m_rel_left, Item->m_rel_top, 0);

        SetRegion(Item->m_rel_left, Item->m_rel_top, Item->m_width, Item->m_height);

        if (Item->Hovered())
            LayoutItem::DrawBox(0, 0, m_cfg.cell_width * Item->m_width, m_cfg.cell_height * Item->m_height, 0xFF004400);
        else
            LayoutItem::DrawBox(0, 0, m_cfg.cell_width * Item->m_width, m_cfg.cell_height * Item->m_height, Item->GetFillColor());

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

    if (m_dragging) {
        int tx, ty, cx, cy;
        GetSelection(tx, ty, cx, cy);
        // Draw Selection rectangle

        // Top
        LayoutItem::DrawBox(tx * m_cfg.cell_width, ty * m_cfg.cell_height - 1, cx * m_cfg.cell_width - 1, m_cfg.border + 1, 0xFF009999);

        // Bottom
        LayoutItem::DrawBox(tx * m_cfg.cell_width, (ty + cy) * m_cfg.cell_height - m_cfg.border - 2, cx * m_cfg.cell_width - 1, m_cfg.border + 2, 0xFF009999);

        // Left
        LayoutItem::DrawBox(tx * m_cfg.cell_width, ty * m_cfg.cell_height, m_cfg.border, cy * m_cfg.cell_height - 1, 0xFF009999);

        // Right
        LayoutItem::DrawBox((tx + cx) * m_cfg.cell_width - m_cfg.border - 2, ty * m_cfg.cell_height, m_cfg.border + 1, cy * m_cfg.cell_height - 1, 0xFF009999);
    }
    endRegion();
}

void Layout::Resize(int target_cx, int target_cy, int cx, int cy)
{
    // We calculate most layout values only on resize here
    m_cfg.canvas_width = target_cx;
    m_cfg.canvas_height = target_cy;

    float ar = float(target_cx) / float(target_cy);

    // TODO: do height first and then calculate target_cx based on that?
    m_cfg.cell_width = float(target_cx) / m_cols;
    m_cfg.cell_height = m_cfg.cell_width / ar;

    target_cy = m_cfg.cell_height * m_rows;

    m_cfg.cx = target_cx;
    m_cfg.cy = target_cy;

    GetScaleAndCenterPos(target_cx, target_cy, cx, cy, m_cfg.x, m_cfg.y, m_cfg.scale);

    m_layout_mutex.lock();
    for (auto& Item : m_layout_items)
        Item->Update(m_cfg);
    m_layout_mutex.unlock();
}

void Layout::RefreshGrid()
{
    auto target_cx = m_cfg.canvas_width;
    auto target_cy = m_cfg.canvas_height;

    float ar = float(target_cx) / float(target_cy);
    m_cfg.cell_width = float(m_cfg.canvas_width) / m_cols;
    m_cfg.cell_height = m_cfg.cell_width / ar;

    target_cy = m_cfg.cell_height * m_rows;

    m_cfg.cx = target_cx;
    m_cfg.cy = target_cy;

    auto* db = static_cast<Durchblick*>(parent());
    auto s = db->size() * db->devicePixelRatioF();
    GetScaleAndCenterPos(target_cx, target_cy, s.width(), s.height(), m_cfg.x, m_cfg.y, m_cfg.scale);

    // Delete any cells that don't fit on the screen anymore
    m_layout_mutex.lock();
    auto it = std::remove_if(m_layout_items.begin(), m_layout_items.end(), [this](std::unique_ptr<LayoutItem> const& item) {
        return item->m_cell.right() >= m_cols + 1 || item->m_cell.bottom() >= m_rows + 1;
    });
    m_layout_items.erase(it, m_layout_items.end());
    FillEmptyCells();

    for (auto& Item : m_layout_items)
        Item->Update(m_cfg);
    m_layout_mutex.unlock();
}

void Layout::CreateDefaultLayout()
{
    m_layout_mutex.lock();
    auto* preview = new PreviewProgramItem(this, 0, 0, 2, 2);
    auto* program = new PreviewProgramItem(this, 2, 0, 2, 2);
    program->SetIsProgram(true);
    program->SetLabel(true);
    program->CreateLabel();
    program->Update(m_cfg);
    preview->SetSafeBorders(true);
    preview->SetLabel(true);
    preview->CreateLabel();
    preview->Update(m_cfg);
    m_layout_items.emplace_back(preview);
    m_layout_items.emplace_back(program);

    struct obs_frontend_source_list scenes = {};

    obs_frontend_get_scenes(&scenes);
    for (int i = 0; i < 8; i++) {
        if (i >= scenes.sources.num) {
            auto* Item = new PlaceholderItem(this, i % 4, i > 3 ? 3 : 2);
            Item->Update(m_cfg);
            m_layout_items.emplace_back(Item);
        } else {
            auto* item = new SceneItem(this, i % 4, i > 3 ? 3 : 2);
            item->SetLabel(true);
            item->SetSource(scenes.sources.array[i]);
            item->Update(m_cfg);
            m_layout_items.emplace_back(item);
        }
    }
    obs_frontend_source_list_free(&scenes);

    m_layout_mutex.unlock();
}

void Layout::Load(QJsonObject const& obj)
{
    m_layout_mutex.lock();
    m_cols = obj["cols"].toInt();
    m_rows = obj["rows"].toInt();
    auto items = obj["items"].toArray();

    for (auto const& item : qAsConst(items)) {
        auto* new_item = Registry::MakeItem(this, item.toObject());
        if (new_item) {
            new_item->Update(m_cfg);
            m_layout_items.emplace_back(new_item);
        } else {
            QJsonDocument doc;
            doc.setObject(item.toObject());
            berr("Failed to instanciate widget '%s'", qt_to_utf8(item.toObject()["id"].toString()));
            berr("Widget JSON: %s", qt_to_utf8(QString(doc.toJson())));
        }
    }
    m_layout_mutex.unlock();
    RefreshGrid();
}

void Layout::Save(QJsonObject& obj)
{
    QJsonArray items;
    obj["cols"] = m_cols;
    obj["rows"] = m_rows;
    for (auto const& Item : m_layout_items) {
        QJsonObject obj;
        Item->WriteToJson(obj);
        items.append(obj);
    }
    obj["items"] = items;
}

void Layout::DeleteLayout()
{
    m_layout_mutex.lock();
    m_layout_items.clear();
    m_layout_mutex.unlock();
}
