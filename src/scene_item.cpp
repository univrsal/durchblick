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
#if _WIN32
#    include <obs-frontend-api.h>
#else
#    include <obs/obs-frontend-api.h>
#endif
#include <util/config-file.h>

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
        OBSSceneAutoRelease s = obs_get_scene_by_name(qt_to_utf8(custom->m_combo_box->currentText()));
        SetSource(obs_scene_get_source(s));
    }
}

void SceneItem::MouseEvent(const MouseData& e, const Config& cfg)
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

static const uint32_t previewColor = 0x7700D000;
static const uint32_t programColor = 0x77D00000;

void SceneItem::Render(const Config& cfg)
{
    SourceItem::Render(cfg);

    OBSSourceAutoRelease previewSrc = obs_frontend_get_current_preview_scene();
    OBSSourceAutoRelease programSrc = obs_frontend_get_current_scene();
    bool studioMode = obs_frontend_preview_program_mode_active();

    auto color = 0;
    if (m_src == programSrc)
        color = programColor;
    else if (m_src == previewSrc)
        color = studioMode ? previewColor : programColor;

    // Draw indicator, that this scene is on preview/program
    if (color != 0) {
        gs_matrix_push();
        gs_matrix_translate3f(cfg.cx / 16, cfg.cy / 16, 0);
        DrawBox(cfg.cx / 32, cfg.cx / 32, color);
        gs_matrix_pop();
    }
}
