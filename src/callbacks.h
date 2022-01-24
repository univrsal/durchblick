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

/**
 * Callback to render custom item content. View region is already adjusted
 * to the cell size. Use 'item' to access additional information about the
 * item.
 * @param item      The layout item object
 * @param data      Private user data
 */
typedef void (*DurchblickItemRenderCb)(void* item, void* data);

/**
 * Mouse move event callback
 * @param item      The layout item object
 * @param data      Private user data
 * @param x         Mouse x position within Durchblick window
 * @param y         Mouse y position within Durchblick window
 * @param buttons   See Qt::MouseButton
 * @param modifiers See Qt::Modifiers
 */
typedef void (*DurchblickItemMouseCb)(void* item, void* data, int x, int y, int buttons, int modifiers);

/**
 * Called to add additional context menu items
 * @param item      The layout item object
 * @param data      Private user data
 * @param event     QContextMenuEvent
 * @param menu      QMenu
 */
typedef void (*DurchBlickItemConextMenuCb)(void* item, void* data, void* event, void* menu);
