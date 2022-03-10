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
#include "item.hpp"

class CustomItem : public LayoutItem {
    Q_OBJECT
    void* m_private_data { nullptr };
    DurchBlickItemConextMenuCb m_context_cb { nullptr };
    DurchblickItemMouseCb m_mouse_cb { nullptr };
    DurchblickItemRenderCb m_render_cb { nullptr };

public:
    CustomItem(Layout* parent, int x, int y, int w = 1, int h = 1, void* priv = nullptr);
    ~CustomItem();

    virtual void ContextMenu(QMenu&) override;
    virtual void MouseEvent(const MouseData& e, const Config& cfg) override;
    virtual void Render(const Config& cfg) override;
};
