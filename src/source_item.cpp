#include "source_item.hpp"
#include "display-helpers.hpp"
#include "layout.hpp"
#include <QApplication>
#include <QMainWindow>
#include <obs/obs-frontend-api.h>

static obs_source_t* placeholder_source = nullptr;
static struct {
    gs_vertbuffer_t* action {};
    gs_vertbuffer_t* graphics {};
    gs_vertbuffer_t* four_by_three {};
    gs_vertbuffer_t* left_line {};
    gs_vertbuffer_t* top_line {};
    gs_vertbuffer_t* right_line {};
} safe_margin = {};

void SourceItem::Init()
{
    obs_data_t* settings = obs_data_create();
    const char* placeholder_path = obs_module_file("placeholder.png");
    obs_data_set_string(settings, "file", placeholder_path);
    placeholder_source = obs_source_create_private("image_source", "durchblick_placeholder", settings);
    bfree((void*)placeholder_path);
    obs_data_release(settings);

    if (!placeholder_source)
        berr("Failed to create placeholder source!");
    obs_enter_graphics();
    InitSafeAreas(&safe_margin.action, &safe_margin.graphics, &safe_margin.four_by_three,
        &safe_margin.left_line, &safe_margin.top_line, &safe_margin.right_line);
    obs_leave_graphics();
}

void SourceItem::Deinit()
{
    obs_enter_graphics();
    gs_vertexbuffer_destroy(safe_margin.action);
    gs_vertexbuffer_destroy(safe_margin.graphics);
    gs_vertexbuffer_destroy(safe_margin.four_by_three);
    gs_vertexbuffer_destroy(safe_margin.left_line);
    gs_vertexbuffer_destroy(safe_margin.top_line);
    gs_vertexbuffer_destroy(safe_margin.right_line);
    obs_leave_graphics();
    obs_source_release(placeholder_source);
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

    m_toggle_safe_borders = new QAction(QCoreApplication::translate("", "Basic.Settings.General.Multiview.DrawSafeAreas"), this);
    m_toggle_safe_borders->setCheckable(true);
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
        obs_source_release(src);
    }
}

void SourceItem::SetSource(obs_source_t* src)
{
    if (!src)
        return;
    if (m_src)
        obs_source_dec_showing(m_src);

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
        if (m_toggle_stretch->isChecked()) {
            gs_matrix_scale3f(m_inner_width / float(w), m_inner_height / float(h), 1);
        } else {
            int x, y;
            float scale;
            GetScaleAndCenterPos(w, h, m_inner_width, m_inner_height, x, y, scale);
            gs_matrix_translate3f(x, y, 0);
            gs_matrix_scale3f(scale, scale, 1);
        }
        obs_source_video_render(m_src);
        if (m_toggle_safe_borders->isChecked()) {
            RenderSafeAreas(safe_margin.action, w, h);
            RenderSafeAreas(safe_margin.graphics, w, h);
            RenderSafeAreas(safe_margin.four_by_three, w, h);
            RenderSafeAreas(safe_margin.left_line, w, h);
            RenderSafeAreas(safe_margin.top_line, w, h);
            RenderSafeAreas(safe_margin.right_line, w, h);
        }
    }
}

void SourceItem::ContextMenu(QMenu& m)
{
    LayoutItem::ContextMenu(m);
    m.addAction(m_toggle_safe_borders);
}
