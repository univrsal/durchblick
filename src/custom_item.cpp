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

#include "custom_item.hpp"

CustomItem::CustomItem(Layout* parent, int x, int y, int w, int h, void* priv)
    : LayoutItem(parent, x, y, w, h)
    , m_private_data(priv)
{
}

CustomItem::~CustomItem()
{
}

void CustomItem::ContextMenu(QContextMenuEvent* e, QMenu& m)
{
    if (m_context_cb)
        m_context_cb(this, m_private_data, e, &m);
}

void CustomItem::MouseEvent(const MouseData& e, const Config& cfg)
{
    if (m_mouse_cb)
        m_mouse_cb(this, m_private_data, e.x, e.y, e.buttons, e.modifiers);
}

void CustomItem::Render(const Config& cfg)
{
    if (m_render_cb)
        m_render_cb(this, m_private_data);
}
