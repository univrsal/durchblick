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

#include "scene_item.hpp"

QWidget* SceneItem::GetConfigWidget()
{
    auto* w = new SceneItemWidget();
    obs_enum_scenes([](void* d, obs_source_t* src) -> bool {
        auto* cb = static_cast<QComboBox*>(d);
        cb->addItem(utf8_to_qt(obs_source_get_name(src)));
        return true;
    },
        w->m_combo_box);
    return w;
}

void SceneItem::LoadConfigFromWidget(QWidget* w)
{
    auto* custom = dynamic_cast<SceneItemWidget*>(w);
    if (custom) {
        auto* src = obs_get_scene_by_name(qt_to_utf8(custom->m_combo_box->currentText()));
        SetSource(obs_scene_get_source(src));
        obs_scene_release(src);
    }
}
