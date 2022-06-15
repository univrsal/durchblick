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
#include <util/config-file.h>

QWidget* SceneItem::GetConfigWidget()
{
    auto* w = new SceneItemWidget();
    QStringList scenes;
    obs_enum_scenes([](void* d, obs_source_t* src) -> bool {
        auto* cb = static_cast<QStringList*>(d);
        cb->append(utf8_to_qt(obs_source_get_name(src)));
        return true;
    },
        &scenes);

    scenes.sort();

    for (auto const& scene : scenes)
        w->m_combo_box->addItem(scene);

    return w;
}

void SceneItem::LoadConfigFromWidget(QWidget* w)
{
    auto* custom = dynamic_cast<SceneItemWidget*>(w);
    if (custom) {
        OBSSceneAutoRelease s = obs_get_scene_by_name(qt_to_utf8(custom->m_combo_box->currentText()));
        m_font_scale = custom->m_font_size->value() / 100.f;
        SetSource(obs_scene_get_source(s));
        if (custom->m_border->isChecked())
            m_indicator_type = Indicator::BORDER;
        else if (custom->m_icon->isChecked())
            m_indicator_type = Indicator::ICON;
        else
            m_indicator_type = Indicator::NONE;
    }
}

void SceneItem::MouseEvent(MouseData const& e, DurchblickItemConfig const& cfg)
{
    SourceItem::MouseEvent(e, cfg);
    auto transitionOnDoubleClick = config_get_bool(
        obs_frontend_get_global_config(), "BasicWindow", "TransitionOnDoubleClick");
    auto switchOnClick = config_get_bool(obs_frontend_get_global_config(), "BasicWindow",
        "MultiviewMouseSwitch");
    if (e.buttons & Qt::LeftButton && Hovered()) {
        if (e.double_click) {
            if (!(obs_frontend_preview_program_mode_active() && transitionOnDoubleClick && switchOnClick))
                return;
            OBSSourceAutoRelease src = obs_frontend_get_current_scene();
            if (src != m_src)
                obs_frontend_set_current_scene(m_src);
        } else {
            if (obs_frontend_preview_program_mode_active()) {
                if (!switchOnClick)
                    return;
                OBSSourceAutoRelease src = obs_frontend_get_current_preview_scene();
                if (src != m_src)
                    obs_frontend_set_current_preview_scene(m_src);
            } else if (switchOnClick) {
                OBSSourceAutoRelease src = obs_frontend_get_current_scene();
                if (src != m_src)
                    obs_frontend_set_current_scene(m_src);
            }
        }
    }
}

void SceneItem::Render(DurchblickItemConfig const& cfg)
{
    SourceItem::Render(cfg);

    if (m_indicator_type == Indicator::ICON) {
        auto color = GetIndicatorColor();
        // Draw indicator, to show that this scene is on preview/program
        if (color != 0) {
            gs_matrix_push();
            gs_matrix_translate3f(cfg.cx / 16, cfg.cy / 16, 0);
            DrawBox(cfg.cx / 32, cfg.cx / 32, color);
            gs_matrix_pop();
        }
    }
}

uint32_t SceneItem::GetFillColor()
{
    if (m_indicator_type == Indicator::BORDER)
        return GetIndicatorColor();
    return LayoutItem::GetFillColor();
}

void SceneItem::ReadFromJson(QJsonObject const& Obj)
{
    SourceItem::ReadFromJson(Obj);
    m_indicator_type = static_cast<Indicator>(Obj["border_for_indicator"].toInt(Indicator::BORDER));
}

void SceneItem::WriteToJson(QJsonObject& Obj)
{
    SourceItem::WriteToJson(Obj);
    Obj["border_for_indicator"] = m_indicator_type;
}
