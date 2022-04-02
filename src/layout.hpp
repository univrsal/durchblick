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
#include "layout_config_dialog.hpp"
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
    friend class LayoutConfigDialog;
    int m_cols { 4 }, m_rows { 4 };
    std::vector<std::unique_ptr<LayoutItem>> m_layout_items;
    LayoutItem::Config m_cfg;
    QWidget* m_parent_widget {};
    QAction *m_new_widget_action {}, *m_layout_config, *m_clear_action {};
    LayoutItem::Cell m_hovered_cell {}, m_selection_start {}, m_selection_end {};
    bool m_dragging {};
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
        NewItemDialog dlg(m_parent_widget, this);
        dlg.exec();
    }

    void ShowLayoutConfigDialog()
    {
        LayoutConfigDialog dlg(m_parent_widget, this);
        dlg.exec();
    }

public:
    Layout(QWidget* parent, int cols = 4, int rows = 4);
    ~Layout();

    void MouseMoved(QMouseEvent* e);
    void MousePressed(QMouseEvent* e);
    void MouseReleased(QMouseEvent* e);
    void MouseDoubleClicked(QMouseEvent* e);
    void HandleContextMenu(QMouseEvent* e);
    void FreeSpace(LayoutItem::Cell const& c);
    void AddWidget(Registry::ItemRegistry::Entry const& entry, QWidget* custom_widget);
    void SetRegion(float bx, float by, float cx, float cy);
    void Render(int target_cx, int target_cy, uint32_t cx, uint32_t cy);
    void Resize(int target_cx, int target_cy, int cx, int cy);
    void RefreshGrid();

    void CreateDefaultLayout();
    void Load(QJsonObject const& obj);
    void Save(QJsonObject& obj);
    bool IsEmpty() const { return m_layout_items.empty(); }
    void DeleteLayout();
    LayoutItem::Config const& Config() const { return m_cfg; }
};
