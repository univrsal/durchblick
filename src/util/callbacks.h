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
#include <jansson.h>

// Provide this when registering your widgets. This makes sure that
// no widgets get registered if the version does not match.
#define DURCHBLICK_CUSTOM_WIDGET_API_VERSION 1

extern "C" {
struct DurchblickItemConfig {
#if defined(__cplusplus)
    int x {}, y {};                        // Origin of multiview
    int cx {}, cy {};                      // Multiview base size (same as canvas size if grid has the same amount of rows and columns)
    int canvas_width {}, canvas_height {}; // Base canvas size
    float scale {};
    float border = 4;
    float border2 = border * 2;
    float cell_width {}, cell_height {};
#else
    int x, y;
    int cx, cy;
    int canvas_width, canvas_height;
    float scale;
    float border;
    float border2;
    float cell_width, cell_height;
#endif
};

/**
 * Callback for when your custom item gets created. Additional layout
 * information is provided in the update call
 * This callback is required.
 * @param item      The layout item object
 * @return          Your private data associated with this widget instance
 */
typedef void* (*DurchblickItemInitCb)(void* item);

/**
 * Simply returns an internal identifying id for this custom widget type
 * This callback is required.
 * @return          The identifier of this widget
 */
typedef const char* (*DurchblickItemIdCb)();

/**
 * Returns the translated display name for this custom widget type
 * This callback is required.
 * @return          The name of this widget
 */
typedef const char* (*DurchblickItemNameCb)();

/**
 * Callback for when your custom item gets destroyed.
 * This callback is required.
 * @param item      The layout item object
 * @param data      Private user data
 */
typedef void (*DurchblickItemDestroyCb)(void* item, void* data);

/**
 * Callback when the widget needs to be updated. Usually happens on window resize
 * @param item      The layout item object
 * @param data      Private user data
 * @param cfg       New widget config data
 * @param col       The column at which this widget starts
 * @param row       The row at which this widget starts
 * @param w         The width in cells of this widget
 * @param h         The height in cells of this widget
 */
typedef void (*DurchblickItemUpdateCb)(void* item, void* data, struct DurchblickItemConfig const* cfg, int col, int row, int w, int h);

/**
 * Callback to save custom data associated with this widget instance.
 * @param  item     The layout item object
 * @param  data     Private user data
 * @return          Jansson json struct populated with your data
 *                  reference will be decreased with json_decref() by Durchblick
 */
typedef json_t* (*DurchblickItemSaveCb)(void* item, void* data);

/**
 * Callback to load custom data associated with this widget instance.
 * @param item      The layout item object
 * @param data      Private user data
 * @param result    Jansson json struct populated with your data
 *                  reference will be decreased with json_decref() by Durchblick
 *                  afterwards.
 * @param size      The size of the info struct
 */
typedef void (*DurchblickItemLoadCb)(void* item, void* data, json_t* result);

/**
 * Callback to render custom item content. View region is already adjusted
 * to the cell size. Use 'item' to access additional information about the
 * item. This callback is required.
 * @param item      The layout item object
 * @param data      Private user data
 * @param cfg       The widget config
 */
typedef void (*DurchblickItemRenderCb)(void* item, void* data, struct DurchblickItemConfig const* cfg);

/**
 * Mouse event callback
 * @param item      The layout item object
 * @param data      Private user data
 * @param cfg       The widget config
 * @param x         Mouse x position within Durchblick window
 * @param y         Mouse y position within Durchblick window
 * @param buttons   See Qt::MouseButton
 * @param modifiers See Qt::Modifiers
 */
typedef void (*DurchblickItemMouseCb)(void* item, void* data, struct DurchblickItemConfig const* cfg, int x, int y, int buttons, int modifiers);

/**
 * Called to add additional context menu items
 * @param item      The layout item object
 * @param data      Private user data
 * @param x         Mouse x position within Durchblick window
 * @param y         Mouse y position within Durchblick window
 * @param buttons   See Qt::MouseButton
 * @param modifiers See Qt::Modifiers
 * @param menu      QMenu
 */
typedef void (*DurchblickItemContextMenuCb)(void* item, void* data, void* menu);

/**
 * The fill color of this cell when it is not hovered. Basically means the
 * border color.
 * @param item      The layout item object
 * @param data      Private user data
 * @return          ARGB 32-bit integer with the fill color
 */
typedef unsigned int (*DurchblickItemFillColorCb)(void* item, void* data);

struct DurchblickCallbacks {
    DurchblickItemInitCb Init;
    DurchblickItemDestroyCb Destroy;
    DurchblickItemSaveCb Save;
    DurchblickItemLoadCb Load;
    DurchblickItemContextMenuCb ContextMenu;
    DurchblickItemMouseCb MouseEvent;
    DurchblickItemRenderCb Render;
    DurchblickItemFillColorCb GetFillColor;
    DurchblickItemUpdateCb Update;
    DurchblickItemIdCb GetId;
    DurchblickItemNameCb GetName;
};
}
