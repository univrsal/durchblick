#include "source_item.hpp"
#include "util.h"
#include "layout.hpp"
#include <obs-frontend-api.h>

static obs_source_t* placeholder_source = nullptr;

static void InitPlaceholder()
{
    obs_data_t *settings = obs_data_create();
    const char* placeholder_path = obs_module_file("placeholder.png");
    obs_data_set_string(settings, "file", placeholder_path);
    placeholder_source = obs_source_create_private("image_source", "durchblick_placeholder", settings);
    bfree((void*)placeholder_path);

    if (!placeholder_source)
        berr("Failed to create placeholder source!");
}

void SourceItem::OBSSourceRemoved(void *data, calldata_t *params)
{
    SourceItem *window = reinterpret_cast<SourceItem*>(data);
    window->m_src = placeholder_source;
}

SourceItem::SourceItem(Layout* parent, int x, int y, int w, int h)
    : LayoutItem(parent, x, y, w, h)
{
//    if (!placeholder_source)
//        InitPlaceholder();
    SetSource(obs_frontend_get_current_scene());
}

SourceItem::~SourceItem()
{
    if (m_src)
        obs_source_dec_showing(m_src);
}

void SourceItem::SetSource(obs_source_t *src)
{
    if (!src)
        return;
    m_src = src;
    if (m_src) {
        removedSignal = OBSSignal(obs_source_get_signal_handler(m_src), "remove",
              SourceItem::OBSSourceRemoved, this);
        obs_source_inc_showing(m_src);
    }
}

void SourceItem::Render(const Config &cfg)
{
    gs_matrix_translate3f(m_rel_left, m_rel_top, 0);
    if (m_mouse_over)
        DrawBox(0, 0, cfg.cell_width * m_width, cfg.cell_height * m_height, 0xFF004400);
    DrawBox(cfg.border, cfg.border, m_inner_width, m_inner_height, 0xFFD0D0D0);
    if (!m_src)
        return;
    if (m_src) {
        int x, y;
        float scale;
        auto w = obs_source_get_width(m_src);
        auto h = obs_source_get_height(m_src);
        GetScaleAndCenterPos(w, h, m_inner_width, m_inner_height, x, y, scale);
        m_layout->SetRegion(cfg.border + x, cfg.border + y, m_inner_width, m_inner_height);

        gs_matrix_translate3f(m_rel_left + cfg.border + x, m_rel_top + cfg.border + y, 0);
        gs_matrix_scale3f(scale, scale, 1);
        obs_source_video_render(m_src);

        endRegion();
    }
}
