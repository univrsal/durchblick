#include "source_item.hpp"
#include "layout.hpp"
#include <obs-frontend-api.h>

static obs_source_t* placeholder_source = nullptr;

void SourceItem::InitPlaceholder()
{
    obs_data_t* settings = obs_data_create();
    const char* placeholder_path = obs_module_file("placeholder.png");
    obs_data_set_string(settings, "file", placeholder_path);
    placeholder_source = obs_source_create_private("image_source", "durchblick_placeholder", settings);
    bfree((void*)placeholder_path);

    if (!placeholder_source)
        berr("Failed to create placeholder source!");
}

void SourceItem::OBSSourceRemoved(void* data, calldata_t* params)
{
    SourceItem* window = reinterpret_cast<SourceItem*>(data);
    window->m_src = placeholder_source;
}

SourceItem::SourceItem(Layout* parent, int x, int y, int w, int h)
    : LayoutItem(parent, x, y, w, h)
{
    SetSource(placeholder_source);
}

SourceItem::~SourceItem()
{
    if (m_src)
        obs_source_dec_showing(m_src);
}

QWidget* SourceItem::GetConfigWidget()
{
    auto* w = new CustomWidget();
    obs_enum_sources([](void* d, obs_source_t* src) -> bool {
        auto flags = obs_source_get_output_flags(src);
        if (flags & OBS_OUTPUT_VIDEO) {
            auto* cb = static_cast<QComboBox*>(d);
            cb->addItem(utf8_to_qt(obs_source_get_name(src)));
        }
        return true;
    },
        w->m_combo_box);
    return w;
}

void SourceItem::LoadConfigFromWidget(QWidget* w)
{
    auto* custom = dynamic_cast<CustomWidget*>(w);
    if (custom) {
        auto* src = obs_get_source_by_name(qt_to_utf8(custom->m_combo_box->currentText()));
        SetSource(src);
    }
}

void SourceItem::SetSource(obs_source_t* src)
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

void SourceItem::Render(const Config& cfg)
{
    LayoutItem::Render(cfg);
    if (!m_src)
        return;
    if (m_src) {
        auto w = obs_source_get_width(m_src);
        auto h = obs_source_get_height(m_src);
        if (cfg.m_fill_cell) {
            gs_matrix_scale3f(m_inner_width / float(w), m_inner_height / float(h), 1);
        } else {
            int x, y;
            float scale;
            GetScaleAndCenterPos(w, h, m_inner_width, m_inner_height, x, y, scale);
            gs_matrix_translate3f(x, y, 0);
            gs_matrix_scale3f(scale, scale, 1);
        }
        obs_source_video_render(m_src);
    }
}
