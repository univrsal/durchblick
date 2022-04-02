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

#include "preview_program_item.hpp"
#include "layout.hpp"
#include <QApplication>
#if _WIN32
#    include <obs-frontend-api.h>
#else
#    include <obs/obs-frontend-api.h>
#endif

QWidget* PreviewProgramItem::GetConfigWidget()
{
    return new PreviewProgramItemWidget;
}

void PreviewProgramItem::LoadConfigFromWidget(QWidget* w)
{
    auto* custom = dynamic_cast<PreviewProgramItemWidget*>(w);
    if (custom)
        m_program = !custom->m_preview->isChecked();

    if (!m_program)
        m_toggle_safe_borders->setChecked(true); // Preview shows safe borders by default
    CreateLabel();
}

void PreviewProgramItem::CreateLabel()
{
    struct obs_video_info ovi;
    obs_get_video_info(&ovi);
    uint32_t h = ovi.base_height;
    QString name = "";
    if (m_program)
        name = QApplication::translate("", T_PROGRAM);
    else
        name = QApplication::translate("", T_PREVIEW);
    m_label = SourceItem::CreateLabel(qt_to_utf8(name), h / 1.5);
}

static const uint32_t labelColor = 0xD91F1F1F;

void PreviewProgramItem::Render(DurchblickItemConfig const& cfg)
{
    LayoutItem::Render(cfg); // Skip SourceItem

    if (!m_src)
        return;
    auto w = cfg.canvas_width;
    auto h = cfg.canvas_height;
    if (m_toggle_stretch->isChecked()) {
        gs_matrix_scale3f(m_inner_width / float(w), m_inner_height / float(h), 1);
    } else {
        int x, y;
        float scale;
        GetScaleAndCenterPos(w, h, m_inner_width, m_inner_height, x, y, scale);
        gs_matrix_translate3f(x, y, 0);
        gs_matrix_scale3f(scale, scale, 1);
    }

    if (m_program || !obs_frontend_preview_program_mode_active()) {
        obs_render_main_texture();
    } else {
        OBSSourceAutoRelease src = obs_frontend_get_current_preview_scene();
        obs_source_video_render(src);
    }

    if (m_toggle_label->isChecked() && m_label) {
        auto lw = obs_source_get_width(m_label);
        auto lh = obs_source_get_height(m_label);
        gs_matrix_push();
        gs_matrix_translate3f((cfg.canvas_width - lw) / 2, cfg.canvas_height * 0.85, 0.0f);
        DrawBox(lw, lh, labelColor);
        gs_matrix_translate3f(0, -(lh * 0.08), 0.0f);
        obs_source_video_render(m_label);
        gs_matrix_pop();
    }
    if (m_toggle_safe_borders->isChecked())
        RenderSafeMargins(w, h);
}

void PreviewProgramItem::WriteToJson(QJsonObject& Obj)
{
    SourceItem::WriteToJson(Obj);
    Obj["is_program"] = m_program;
}

void PreviewProgramItem::ReadFromJson(QJsonObject const& Obj)
{
    SourceItem::ReadFromJson(Obj);
    m_program = Obj["is_program"].toBool();
    if (m_toggle_label->isChecked())
        CreateLabel();
}
