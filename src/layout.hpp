/*************************************************************************
 * This file is part of input-overlay
 * git.vrsal.xyz/alex/durchblick
 * Copyright 2021 univrsal <uni@vrsal.xyz>.
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

#include <vector>
#include <memory>
#include <obs-module.h>

inline void startRegion(int vX, int vY, int vCX, int vCY, float oL,
                   float oR, float oT, float oB)
{
    gs_projection_push();
    gs_viewport_push();
    gs_set_viewport(vX, vY, vCX, vCY);
    gs_ortho(oL, oR, oT, oB, -100.0f, 100.0f);
}

inline void endRegion()
{
    gs_viewport_pop();
    gs_projection_pop();
}

inline void GetScaleAndCenterPos(int baseCX, int baseCY, int windowCX,
                    int windowCY, int &x, int &y,
                    float &scale)
{
    double windowAspect, baseAspect;
    int newCX, newCY;

    windowAspect = double(windowCX) / double(windowCY);
    baseAspect = double(baseCX) / double(baseCY);

    if (windowAspect > baseAspect) {
        scale = float(windowCY) / float(baseCY);
        newCX = int(double(windowCY) * baseAspect);
        newCY = windowCY;
    } else {
        scale = float(windowCX) / float(baseCX);
        newCX = windowCX;
        newCY = int(float(windowCX) / baseAspect);
    }

    x = windowCX / 2 - newCX / 2;
    y = windowCY / 2 - newCY / 2;
}

// Default item, renders black rectangle
class LayoutItem {
public:
    int m_column{}, m_row{}, m_width{}, m_height{};
    struct Config {
        float border = 4;
        float border2 = border * 2;
        float cell_width, cell_height;
    };

    LayoutItem(int x, int y, int w = 1, int h = 1)
        : m_column(x)
        , m_row(y)
        , m_width(w)
        , m_height(h)
    {

    }

    virtual void Render(Config const& cfg)
    {
        DrawBox(cfg.border, cfg.border, cfg.cell_width * m_width - cfg.border2,
                cfg.cell_height * m_height - cfg.border2, 0xFF000000);
    }

    static void DrawBox(float cx, float cy, uint32_t colorVal) {
        gs_effect_t *solid = obs_get_base_effect(OBS_EFFECT_SOLID);
        gs_eparam_t *color =
            gs_effect_get_param_by_name(solid, "color");

        gs_effect_set_color(color, colorVal);
        while (gs_effect_loop(solid, "Solid"))
            gs_draw_sprite(nullptr, 0, (uint32_t)cx, (uint32_t)cy);
    }

    static void DrawBox(float tx, float ty, float cx, float cy, uint32_t color)
    {
        gs_matrix_push();
        gs_matrix_translate3f(tx, ty, 0.0f);
        DrawBox(cx, cy, color);
        gs_matrix_pop();
    };
};

class Layout {
    int m_size;
    std::vector<std::unique_ptr<LayoutItem>> m_layout_items;
    LayoutItem::Config m_cfg;
public:
    Layout(int size = 4)
        : m_size(size)
    {
        for (int x = 0; x < size; x++) {
            for (int y = 0; y < size; y++) {
                m_layout_items.emplace_back(new LayoutItem(x, y));
            }
        }
    }

    void Render(int target_cx, int target_cy, uint32_t cx, uint32_t cy)
    {
        int x, y;
        float scale;
        GetScaleAndCenterPos(target_cx, target_cy, cx, cy, x, y, scale);

        m_cfg.cell_width = float(target_cx) / m_size;
        m_cfg.cell_height = float(target_cy) / m_size;
        // Define the whole usable region for the multiview
        startRegion(x, y, target_cx * scale, target_cy * scale, 0.0f, target_cx,
                    0.0f, target_cy);
        LayoutItem::DrawBox(target_cx, target_cy, 0xFFD0D0D0);

        auto setRegion = [&](float bx, float by, float cx, float cy) {
            float vX = int(x + bx * scale);
            float vY = int(y + by * scale);
            float vCX = int(cx * scale);
            float vCY = int(cy * scale);

            float oL = bx;
            float oT = by;
            float oR = (bx + cx);
            float oB = (by + cy);
            startRegion(vX, vY, vCX, vCY, oL, oR, oT, oB);
        };
        for (auto& Item : m_layout_items) {
            // Change region to item dimensions
            setRegion(Item->m_column * m_cfg.cell_width, Item->m_row * m_cfg.cell_height,
                      Item->m_width * m_cfg.cell_width, Item->m_height * m_cfg.cell_height);
            gs_matrix_push();
            gs_matrix_translate3f(Item->m_column * m_cfg.cell_width, Item->m_row * m_cfg.cell_height, 0);
            Item->Render(m_cfg);
            gs_matrix_pop();
            endRegion();
        }
        endRegion();
    }
};
