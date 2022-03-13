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
#include <obs-module.h>

#define write_log(log_level, format, ...) blog(log_level, "[durchblick] " format, ##__VA_ARGS__)

#define bdebug(format, ...) write_log(LOG_DEBUG, format, ##__VA_ARGS__)
#define binfo(format, ...) write_log(LOG_INFO, format, ##__VA_ARGS__)
#define bwarn(format, ...) write_log(LOG_WARNING, format, ##__VA_ARGS__)
#define berr(format, ...) write_log(LOG_ERROR, format, ##__VA_ARGS__)

#define utf8_to_qt(_str) QString::fromUtf8(_str)
#define qt_to_utf8(_str) _str.toUtf8().constData()

/* clang-format off */

/* Misc */
#define T_(v)                           obs_module_text(v)
#define T_MENU_OPTION                   T_("Menu.Option")
#define T_MENU_SET_WIDGET               T_("Menu.SetWidget")
#define T_MENU_LAYOUT_CONFIG            T_("Menu.LayoutConfig")
#define T_SELECT_TYPE_DIALOG            T_("Dialog.Select.ItemType")
#define T_SELECT_TYPE                   T_("Label.Select.ItemType")
#define T_WIDGET_SETTINGS               T_("Label.WidgetSettings")
#define T_SOURCE_NAME                   T_("Label.SourceName")
#define T_SCENE_NAME                    T_("Label.SceneName")
#define T_LABEL_GRID_SIZE               T_("Label.GridSize")
#define T_LAYOUT_CONFIG                 T_("LayoutConfig.Title")
#define T_WIDGET_STRETCH                T_("Widget.Stretch")
#define T_SOURCE_ITEM_LABEL             T_("SourceItem.Label")
#define T_SOURCE_ITEM_VOLUME            T_("SourceItem.Volume")
#define T_WIDGET_SOURCE                 T_("Widget.SourceDisplay")
#define T_WIDGET_SCENE                  T_("Widget.SceneDisplay")
#define T_WIDGET_PREVIEW_PROGRAM        T_("Widget.PreviewProgramDisplay")

#define T_PREVIEW                       "StudioMode.Preview"
#define T_PROGRAM                       "StudioMode.Program"
/* clang-format on */
