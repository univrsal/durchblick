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

#include "items/item.hpp"
#include "items/registry.hpp"
#include "ui/layout_config_dialog.hpp"
#include "ui/new_item_dialog.hpp"
#include <QMouseEvent>
#include <algorithm>
#include <memory>
#include <mutex>
#include <obs-module.h>
#include <vector>

inline void StartRegion(int vX, int vY, int vCX, int vCY, float oL,
    float oR, float oT, float oB)
{
    gs_projection_push();
    gs_viewport_push();
    gs_set_viewport(vX, vY, vCX, vCY);
    gs_ortho(oL, oR, oT, oB, -100.0f, 100.0f);
}

inline void EndRegion()
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
    friend class LayoutConfigDialog;
    int m_cols { 4 }, m_rows { 4 };
    std::vector<std::unique_ptr<LayoutItem>> m_layout_items;
    DurchblickItemConfig m_cfg;
    Durchblick* m_durchblick {};
    LayoutItem::Cell m_hovered_cell {}, m_selection_start {}, m_selection_end {};
    bool m_dragging {}, m_locked {};
    std::mutex m_layout_mutex;
    Q_OBJECT

    void GetSelection(int& tx, int& ty, int& cx, int& cy)
    {
        tx = qMin(m_selection_start.left(), m_selection_end.left());
        ty = qMin(m_selection_start.top(), m_selection_end.top());
        cx = qAbs(qMax(m_selection_start.right(), m_selection_end.right())) - tx;
        cy = qAbs(qMax(m_selection_start.bottom(), m_selection_end.bottom())) - ty;
    }

    void FillEmptyCells();

    LayoutItem::Cell GetSelectedArea();
private slots:

    void ClearSelection();
    void ShowSetWidgetDialog()
    {
        NewItemDialog dlg(m_durchblick, this);
        dlg.exec();
    }

    void ShowLayoutConfigDialog()
    {
        LayoutConfigDialog dlg(m_durchblick, this);
        dlg.exec();
    }

    void Lock() { m_locked = true; }
    void Unlock() { m_locked = false; }

    void FillSelectionWithScenes();

public:
    Layout(Durchblick* parent, int cols = 4, int rows = 4);
    ~Layout();

    void MouseMoved(QMouseEvent* e);
    void MousePressed(QMouseEvent* e);
    void MouseReleased(QMouseEvent* e);
    void MouseDoubleClicked(QMouseEvent* e);
    void HandleContextMenu(QMouseEvent* e, QMenu& m);
    void FreeSpace(LayoutItem::Cell const& c);
    void AddWidget(Registry::ItemRegistry::Entry const& entry, LayoutItem::Cell const& c, QWidget* custom_widget);
    void AddWidget(Registry::ItemRegistry::Entry const& entry, QWidget* custom_widget);
    void SetRegion(float bx, float by, float cx, float cy);
    void Render(int target_cx, int target_cy, uint32_t cx, uint32_t cy);
    void Resize(int target_cx, int target_cy, int cx, int cy);
    void RefreshGrid();

    void CreateDefaultLayout();
    void Load(QJsonObject const& obj);
    void Save(QJsonObject& obj);
    bool IsEmpty() const { return m_layout_items.empty(); }
    bool IsLocked() const { return m_locked; }
    void DeleteLayout();
    void ResetHover();
    void Clear()
    {
        std::lock_guard<std::mutex> lock(m_layout_mutex);
        m_layout_items.clear();
    }

    int Columns() const { return m_cols; }
    int Rows() const { return m_rows; }
    DurchblickItemConfig const& Config() const { return m_cfg; }
};
