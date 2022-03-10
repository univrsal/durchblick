#include "source_item.hpp"
#include "display-helpers.hpp"
#include "layout.hpp"
#include <QApplication>
#include <QMainWindow>
#include <obs/obs-frontend-api.h>

/* yoinked from obs window-projector.cpp */
OBSSource SourceItem::CreateLabel(const char* name, size_t h)
{
    OBSDataAutoRelease settings = obs_data_create();
    OBSDataAutoRelease font = obs_data_create();

    std::string text;
    text += " ";
    text += name;
    text += " ";

#if defined(_WIN32)
    obs_data_set_string(font, "face", "Arial");
#elif defined(__APPLE__)
    obs_data_set_string(font, "face", "Helvetica");
#else
    obs_data_set_string(font, "face", "Monospace");
#endif
    obs_data_set_int(font, "flags", 1); // Bold text
    obs_data_set_int(font, "size", int(h / 9.81));

    obs_data_set_obj(settings, "font", font);
    obs_data_set_string(settings, "text", text.c_str());
    obs_data_set_bool(settings, "outline", false);

#ifdef _WIN32
    const char* text_source_id = "text_gdiplus";
#else
    const char* text_source_id = "text_ft2_source";
#endif

    OBSSourceAutoRelease txtSource = obs_source_create_private(text_source_id, name, settings);

    return txtSource.Get();
}

static obs_source_t* placeholder_source = nullptr;
static struct {
    gs_vertbuffer_t* action {};
    gs_vertbuffer_t* graphics {};
    gs_vertbuffer_t* four_by_three {};
    gs_vertbuffer_t* left_line {};
    gs_vertbuffer_t* top_line {};
    gs_vertbuffer_t* right_line {};
} safe_margin = {};

void SourceItem::RenderSafeMargins(int w, int h)
{
    RenderSafeAreas(safe_margin.action, w, h);
    RenderSafeAreas(safe_margin.graphics, w, h);
    RenderSafeAreas(safe_margin.four_by_three, w, h);
    RenderSafeAreas(safe_margin.left_line, w, h);
    RenderSafeAreas(safe_margin.top_line, w, h);
    RenderSafeAreas(safe_margin.right_line, w, h);
}

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

    m_toggle_safe_borders = new QAction(QCoreApplication::translate("", "Basic.Settings.General.Multiview.DrawSafeAreas"), this);
    m_toggle_safe_borders->setCheckable(true);
    m_toggle_label = new QAction(T_SOURCE_ITEM_LABEL, this);
    m_toggle_label->setCheckable(true);
    SetSource(placeholder_source);
    m_toggle_label->setChecked(true);
}

SourceItem::~SourceItem()
{
    if (m_src)
        obs_source_dec_showing(m_src);
}

QWidget* SourceItem::GetConfigWidget()
{
    auto* w = new SourceItemWidget();
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
    auto* custom = dynamic_cast<SourceItemWidget*>(w);
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
        if (m_toggle_label->isChecked()) {
            struct obs_video_info ovi;
            obs_get_video_info(&ovi);

            uint32_t h = ovi.base_height;
            m_label = CreateLabel(obs_source_get_name(m_src), h / 1.5);
        }
    }
}

static const uint32_t outerColor = 0xFFD0D0D0;
static const uint32_t labelColor = 0xD91F1F1F;
static const uint32_t backgroundColor = 0xFF000000;
static const uint32_t previewColor = 0xFF00D000;
static const uint32_t programColor = 0xFFD00000;

void SourceItem::Render(const Config& cfg)
{
    LayoutItem::Render(cfg);

    if (!m_src)
        return;
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

    if (m_toggle_label->isChecked() && m_label) {
        auto lw = obs_source_get_width(m_label);
        auto lh = obs_source_get_height(m_label);
        gs_matrix_push();
        gs_matrix_translate3f((cfg.cx - lw) / 2, cfg.cy * 0.85, 0.0f);
        DrawBox(lw, lh, labelColor);
        gs_matrix_translate3f(0, -(lh * 0.08), 0.0f);
        obs_source_video_render(m_label);
        gs_matrix_pop();
    }
    if (m_toggle_safe_borders->isChecked())
        RenderSafeMargins(w, h);
}

void SourceItem::ContextMenu(QMenu& m)
{
    LayoutItem::ContextMenu(m);
    m.addAction(m_toggle_safe_borders);
    m.addAction(m_toggle_label);
}
