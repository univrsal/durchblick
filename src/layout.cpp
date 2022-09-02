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
#include "items/preview_program_item.hpp"
#include "items/scene_item.hpp"
#include "ui/durchblick.hpp"
#include "util/util.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <obs-frontend-api.h>
#include <util/config-file.h>

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

void Layout::FillSelectionWithScenes()
{
    {
        std::lock_guard<std::mutex> lock(m_layout_mutex);
        struct obs_frontend_source_list scenes = {};
        obs_frontend_get_scenes(&scenes);

        auto target = GetSelectedArea();
        auto cells_to_fill = target.w * target.h;
        std::vector<std::string> existing_scenes;
        FreeSpace(target);

        // Get all scenes that are already in the layout
        for (auto const& item : m_layout_items) {
            auto* si = dynamic_cast<SceneItem*>(item.get());
            if (si && si->GetSource().Get())
                existing_scenes.emplace_back(obs_source_get_name(si->GetSource()));
        }

        auto* ItemEntry = Registry::GetEntryById("SceneItem");
        if (!ItemEntry)
            return;

        int col {}, row {};
        for (int i = 0; i < int(scenes.sources.num) && cells_to_fill > 0; i++) {
            auto* src = scenes.sources.array[i];

            // Don't add scenes that shouldn't be shown in the normal multiview
            OBSDataAutoRelease data = obs_source_get_private_settings(src);
            obs_data_set_default_bool(data, "show_in_multiview", true);
            if (!obs_data_get_bool(data, "show_in_multiview"))
                continue;

            std::string n(obs_source_get_name(src));
            auto exists = std::find_if(existing_scenes.begin(), existing_scenes.end(), [n](std::string const& name) {
                return n == name;
            });
            if (exists != existing_scenes.end())
                continue;

            LayoutItem::Cell c;
            c.col = target.col + col;
            c.row = target.row + row;

            col++;
            if (col >= target.w) {
                col = 0;
                row++;
            }

            auto* Item = new SceneItem(this, c.col, c.row);
            Item->Update(m_cfg);
            Item->SetSource(src);
            FreeSpace(c);
            m_layout_items.emplace_back(Item);
            cells_to_fill--;
        }
        FillEmptyCells();
        obs_frontend_source_list_free(&scenes);
    }
    Config::Save();
}

Layout::Layout(Durchblick* parent, int cols, int rows)
    : QObject(parent)
    , m_cols(cols)
    , m_rows(rows)
    , m_durchblick(parent)
{
}

Layout::~Layout()
{
}

void Layout::MouseMoved(QMouseEvent* e)
{
    LayoutItem::MouseData d(
        int((e->pos().x() - m_cfg.x) / m_cfg.scale),
        int((e->pos().y() - m_cfg.y) / m_cfg.scale),
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
        int((e->pos().x() - m_cfg.x) / m_cfg.scale),
        int((e->pos().y() - m_cfg.y) / m_cfg.scale),
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
        int((e->pos().x() - m_cfg.x) / m_cfg.scale),
        int((e->pos().y() - m_cfg.y) / m_cfg.scale),
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
        int((e->pos().x() - m_cfg.x) / m_cfg.scale),
        int((e->pos().y() - m_cfg.y) / m_cfg.scale),
        e->modifiers(),
        e->buttons(),
        e->type());
    d.double_click = true;
    for (auto& Item : m_layout_items)
        Item->MouseEvent(d, m_cfg);
}

void Layout::HandleContextMenu(QMouseEvent*, QMenu& m)
{
    if (m_locked) {
        m.addAction(T_MENU_UNLOCK, this, SLOT(Unlock()));
    } else {

        // Keep drawing the selection if it wasn't reset
        if (!m_selection_end.empty())
            m_dragging = true;

        m.addAction(T_MENU_CONFIGURATION, this, SLOT(ShowLayoutConfigDialog()));
        m.addAction(T_MENU_LOCK, this, SLOT(Lock()));
        std::lock_guard<std::mutex> lock(m_layout_mutex);

        for (auto& Item : m_layout_items) {
            if (Item->Hovered()) {
                auto* sub_menu = m.addMenu(T_MENU_QUICK_ACTIONS);
                sub_menu->addAction(T_MENU_FILL_ACTION, this, SLOT(FillSelectionWithScenes()));
                sub_menu->addAction(T_MENU_CLEAR_ACTION, this, SLOT(ClearSelection()));
                m.addAction(T_MENU_SET_WIDGET, this, SLOT(ShowSetWidgetDialog()));
                m.addSeparator();
                Item->ContextMenu(m);
                break;
            }
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

void Layout::AddWidget(Registry::ItemRegistry::Entry const& entry, const LayoutItem::Cell& c, QWidget* custom_widget)
{
    auto* Item = entry.construct(this, c.col, c.row, c.w, c.h);
    Item->LoadConfigFromWidget(custom_widget);
    Item->Update(m_cfg);

    m_layout_mutex.lock();
    FreeSpace(c);
    m_layout_items.emplace_back(Item);
    FillEmptyCells();
    m_layout_mutex.unlock();

    Config::Save();
}

void Layout::AddWidget(Registry::ItemRegistry::Entry const& entry, QWidget* custom_widget)
{
    AddWidget(entry, GetSelectedArea(), custom_widget);
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

void Layout::Render(int, int, uint32_t, uint32_t)
{
    // Define the whole usable region for the multiview
    startRegion(m_cfg.x, m_cfg.y, m_cfg.cx * m_cfg.scale, m_cfg.cy * m_cfg.scale, 0.0f, m_cfg.cx,
        0.0f, m_cfg.cy);
    LayoutItem::DrawBox(m_cfg.cx, m_cfg.cy, COLOR_BORDER_GRAY);

    m_layout_mutex.lock();
    for (auto& Item : m_layout_items) {
        // Change region to item dimensions
        gs_matrix_push();
        gs_matrix_translate3f(Item->m_rel_left, Item->m_rel_top, 0);

        SetRegion(Item->m_rel_left, Item->m_rel_top, Item->m_width, Item->m_height);

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
        LayoutItem::DrawBox(tx * m_cfg.cell_width, ty * m_cfg.cell_height - 1, cx * m_cfg.cell_width - 1, m_cfg.border + 1, COLOR_SELECTION_CYAN);

        // Bottom
        LayoutItem::DrawBox(tx * m_cfg.cell_width, (ty + cy) * m_cfg.cell_height - m_cfg.border - 2, cx * m_cfg.cell_width - 1, m_cfg.border + 2, COLOR_SELECTION_CYAN);

        // Left
        LayoutItem::DrawBox(tx * m_cfg.cell_width, ty * m_cfg.cell_height, m_cfg.border, cy * m_cfg.cell_height - 1, COLOR_SELECTION_CYAN);

        // Right
        LayoutItem::DrawBox((tx + cx) * m_cfg.cell_width - m_cfg.border - 2, ty * m_cfg.cell_height, m_cfg.border + 1, cy * m_cfg.cell_height - 1, COLOR_SELECTION_CYAN);
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
        if (i >= int(scenes.sources.num)) {
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
    m_layout_mutex.unlock();
    obs_frontend_source_list_free(&scenes);

    auto cfg = obs_frontend_get_global_config();

    // Automatically set settings to user default
    m_durchblick->SetHideFromDisplayCapture(config_get_bool(cfg, "BasicWindow", "HideOBSWindowsFromCapture"));

    m_durchblick->SetHideCursor(config_get_bool(cfg, "BasicWindow", "HideProjectorCursor"));
    m_durchblick->SetIsAlwaysOnTop(config_get_bool(cfg, "BasicWindow", "ProjectorAlwaysOnTop"));
}

void Layout::Load(QJsonObject const& obj)
{
    Clear();

    m_layout_mutex.lock();
    m_cols = obj["cols"].toInt();
    m_rows = obj["rows"].toInt();
    m_locked = obj["locked"].toBool();
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
    if (IsEmpty())
        CreateDefaultLayout();

    RefreshGrid();
}

void Layout::Save(QJsonObject& obj)
{
    std::lock_guard<std::mutex> lock(m_layout_mutex);
    QJsonArray items;
    obj["cols"] = m_cols;
    obj["rows"] = m_rows;
    obj["locked"] = m_locked;
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

void Layout::ResetHover()
{
    if (m_hovered_cell.col > -1) {
        m_hovered_cell.col = -1;
        m_hovered_cell.row = -1;
        auto pos = m_durchblick->mapFromGlobal(QCursor::pos());
        for (auto const& i : m_layout_items) {
            i->IsMouseOver(pos.x(), pos.y());
        }
    }
}
