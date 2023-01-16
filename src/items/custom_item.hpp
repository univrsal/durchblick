/*************************************************************************
 * This file is part of durchblick
 * git.vrsal.xyz/alex/durchblick
 * Copyright 2023 univrsal <uni@vrsal.xyz>.
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

#include "../util/callbacks.h"
#include "item.hpp"

class CustomItem : public LayoutItem {
    Q_OBJECT
private:
    DurchblickCallbacks m_cb_data {};
    void* PrivateData { nullptr };

public:
    CustomItem(Layout* parent, DurchblickCallbacks const& cbs, int x, int y, int w = 1, int h = 1);
    ~CustomItem();

    void Update(DurchblickItemConfig const& cfg) override;
    void ContextMenu(QMenu&) override;
    void MouseEvent(MouseData const& e, DurchblickItemConfig const& cfg) override;
    void Render(DurchblickItemConfig const& cfg) override;
    void WriteToJson(QJsonObject& Obj) override;
    void ReadFromJson(QJsonObject const& Obj) override;
    uint32_t GetFillColor() override;
};
