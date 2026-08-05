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

#include "preview_program_item.hpp"
#include "../layout.hpp"
#include <QApplication>
#include <obs-frontend-api.h>

QWidget* PreviewProgramItem::GetConfigWidget()
{
    return new PreviewProgramItemWidget;
}

void PreviewProgramItem::SetIsProgram(bool program)
{
    m_program = program;
    if (m_program && !m_output_meter)
        m_output_meter = std::make_unique<OutputMeter>();
    else if (!m_program)
        m_output_meter.reset();
}

void PreviewProgramItem::LoadConfigFromWidget(QWidget* w)
{
    auto* custom = dynamic_cast<PreviewProgramItemWidget*>(w);
    if (custom) {
        SetIsProgram(!custom->m_preview->isChecked());
        m_font_scale = custom->m_font_size->value() / 100.f;
    }

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
        name = T_PROGRAM;
    else
        name = T_PREVIEW;
    m_label = ::CreateLabel(qt_to_utf8(name), h / 1.5, m_font_scale);
}

static const uint32_t labelColor = 0xD91F1F1F;

void PreviewProgramItem::Render(DurchblickItemConfig const& cfg)
{
    LayoutItem::Render(cfg); // Skip SourceItem

    if (!m_src)
        return;
    auto w = cfg.canvas_width;
    auto h = cfg.canvas_height;
    gs_matrix_push();
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

        if (lw >= 30 && lh >= 10) { // No reason to draw an unreadable label
            gs_matrix_push();
            gs_matrix_translate3f((cfg.canvas_width - lw) / 2, cfg.canvas_height - lh * 1.5, 0.0f);
            DrawBox(lw, lh, labelColor);
            gs_matrix_translate3f(0, -(lh * 0.08), 0.0f);
            obs_source_video_render(m_label);
            gs_matrix_pop();
        }
    }
    if (m_toggle_safe_borders->isChecked())
        RenderSafeMargins(w, h);
    gs_matrix_pop();

    if (m_output_meter) {
        const float cell_scale = qMax(0.001f, cfg.scale);
        const int meter_margin = qMax(1, int(8.0f / cell_scale));
        const int meter_width = m_output_meter->GetRenderWidth(cell_scale);
        const int meter_height = qMax(1, int(m_inner_height) - meter_margin * 2);
        m_output_meter->SetHeight(meter_height);
        m_output_meter->SetPos(
            qMax(0, int(m_inner_width) - meter_width - meter_margin),
            meter_margin);
        DrawBox(m_output_meter->GetX() - 3, meter_margin - 3,
            meter_width + 6, meter_height + 6, 0xB0000000);
        m_output_meter->Render(cell_scale, 1.0f, 1.0f);
    }
}

void PreviewProgramItem::WriteToJson(QJsonObject& Obj)
{
    SourceItem::WriteToJson(Obj);
    Obj["is_program"] = m_program;
}

void PreviewProgramItem::ReadFromJson(QJsonObject const& Obj)
{
    SourceItem::ReadFromJson(Obj);
    SetIsProgram(Obj["is_program"].toBool());
    if (m_toggle_label->isChecked())
        CreateLabel();
}
