#include "source_item.hpp"
#include "../layout.hpp"
#include "../util/display_helpers.hpp"
#include <QApplication>
#include <QMainWindow>
#include <obs-frontend-api.h>
#include <util/util.hpp>

/* yoinked from obs window-projector.cpp */
OBSSource SourceItem::CreateLabel(char const* name, size_t h, float scale)
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
    obs_data_set_int(font, "size", int(h / 9.81) * scale);

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

void SourceItem::VolumeToggled(bool state)
{
    if (state && m_src) {
        auto h = obs_source_get_height(m_src);
        m_vol_meter = std::make_unique<VolumeMeter>(m_src, m_volume_meter_x, m_volume_meter_y, h / 2);
    } else {
        if (m_vol_meter) {
            m_volume_meter_x = m_vol_meter->get_x();
            m_volume_meter_y = m_vol_meter->get_y();
        }
        m_vol_meter = nullptr;
    }
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
    OBSDataAutoRelease settings = obs_data_create();
    BPtr<char> placeholder_path = obs_module_file("placeholder.png");
    obs_data_set_string(settings, "file", placeholder_path);
    placeholder_source = obs_source_create_private("image_source", "durchblick_placeholder", settings);

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

void SourceItem::OBSSourceRemoved(void* data, calldata_t*)
{
    SourceItem* window = reinterpret_cast<SourceItem*>(data);
    window->m_src = placeholder_source;
    if (window->m_vol_meter)
        window->m_vol_meter->set_source(placeholder_source);
}

SourceItem::SourceItem(Layout* parent, int x, int y, int w, int h)
    : LayoutItem(parent, x, y, w, h)
{

    m_toggle_safe_borders = new QAction(T_DRAW_SAFE_BORDERS, this);
    m_toggle_safe_borders->setCheckable(true);
    m_toggle_label = new QAction(T_SOURCE_ITEM_LABEL, this);
    m_toggle_label->setCheckable(true);
    m_toggle_volume = new QAction(T_SOURCE_ITEM_VOLUME, this);
    m_toggle_volume->setCheckable(true);
    SetSource(placeholder_source);
    m_toggle_label->setChecked(true);
    connect(m_toggle_volume, SIGNAL(toggled(bool)), this, SLOT(VolumeToggled(bool)));
}

SourceItem::~SourceItem()
{
    if (m_src)
        obs_source_dec_showing(m_src);
}

QWidget* SourceItem::GetConfigWidget()
{
    auto* w = new SourceItemWidget();
    QStringList names;

    obs_enum_sources([](void* d, obs_source_t* src) -> bool {
        auto flags = obs_source_get_output_flags(src);
        if (flags & OBS_OUTPUT_VIDEO) {
            auto* cb = static_cast<QStringList*>(d);
            cb->append(utf8_to_qt(obs_source_get_name(src)));
        }
        return true;
    },
        &names);
    names.sort();
    for (auto const& name : names)
        w->m_combo_box->addItem(name);

    return w;
}

void SourceItem::LoadConfigFromWidget(QWidget* w)
{
    auto* custom = dynamic_cast<SourceItemWidget*>(w);
    if (custom) {
        OBSSourceAutoRelease src = obs_get_source_by_name(qt_to_utf8(custom->m_combo_box->currentText()));
        m_font_scale = custom->m_font_size->value() / 100.f;
        m_channel_width = custom->m_channel_width->value();
        m_volume_meter_height = custom->m_volume_meter_height->value() / 100.f;
        m_toggle_volume->setChecked(custom->m_show_volume_meter->isChecked());
        if (src && custom->m_show_volume_meter->isChecked()) {
            auto h = obs_source_get_height(src);
            m_vol_meter = std::make_unique<VolumeMeter>(src.Get(), 10, 10, int(h * m_volume_meter_height));
        }
        SetSource(src);
    }
}

void SourceItem::SetSource(obs_source_t* src)
{
    if (m_src)
        obs_source_dec_showing(m_src);

    m_src = src;
    if (m_src) {
        if (m_vol_meter)
            m_vol_meter->set_source(src);
        removedSignal = OBSSignal(obs_source_get_signal_handler(m_src), "remove",
            SourceItem::OBSSourceRemoved, this);
        obs_source_inc_showing(m_src);
        if (m_toggle_label->isChecked()) {
            struct obs_video_info ovi;
            obs_get_video_info(&ovi);

            uint32_t h = ovi.base_height;
            m_label = CreateLabel(obs_source_get_name(m_src), h / 1.5, m_font_scale);
        }
    }
}

void SourceItem::ReadFromJson(QJsonObject const& Obj)
{
    LayoutItem::ReadFromJson(Obj);
    m_toggle_safe_borders->setChecked(Obj["show_safe_borders"].toBool());
    m_toggle_label->setChecked(Obj["show_label"].toBool());
    m_toggle_volume->setChecked(Obj["show_volume"].toBool());

    if (Obj["font_scale"].isDouble())
        m_font_scale = Obj["font_scale"].toDouble(1);

    if (Obj["volume_meter_channel_width"].isDouble())
        m_channel_width = Obj["volume_meter_channel_width"].toInt(2);

    if (Obj["volume_meter_height"].isDouble())
        m_volume_meter_height = Obj["volume_meter_height"].toDouble(.5);

    if (Obj["volume_meter_x"].isDouble())
        m_volume_meter_x = Obj["volume_meter_x"].toDouble(10);
    if (Obj["volume_meter_y"].isDouble())
        m_volume_meter_y = Obj["volume_meter_y"].toDouble(10);

    OBSSourceAutoRelease src = obs_get_source_by_name(qt_to_utf8(Obj["source"].toString()));
    if (src)
        SetSource(src);
    else
        SetSource(placeholder_source);

    if (src.Get() && m_toggle_volume->isChecked()) {
        auto h = obs_source_get_height(src);
        m_vol_meter = std::make_unique<VolumeMeter>(src.Get(), m_volume_meter_x, m_volume_meter_y, int(h * m_volume_meter_height));
    }
}

void SourceItem::WriteToJson(QJsonObject& Obj)
{
    LayoutItem::WriteToJson(Obj);
    if (m_src)
        Obj["source"] = utf8_to_qt(obs_source_get_name(m_src));
    Obj["show_safe_borders"] = m_toggle_safe_borders->isChecked();
    Obj["show_label"] = m_toggle_label->isChecked();
    Obj["show_volume"] = m_toggle_volume->isChecked();
    Obj["font_scale"] = m_font_scale;
    Obj["volume_meter_channel_width"] = m_channel_width;
    Obj["volume_meter_height"] = m_volume_meter_height;

    if (m_vol_meter) {
        Obj["volume_meter_x"] = m_vol_meter->get_x();
        Obj["volume_meter_y"] = m_vol_meter->get_y();
    } else {
        Obj["volume_meter_x"] = m_volume_meter_x;
        Obj["volume_meter_y"] = m_volume_meter_y;
    }
}

static const uint32_t outerColor = 0xFFD0D0D0;
static const uint32_t labelColor = 0xD91F1F1F;
static const uint32_t backgroundColor = 0xFF000000;
static const uint32_t previewColor = 0xFF00D000;
static const uint32_t programColor = 0xFFD00000;

void SourceItem::Render(DurchblickItemConfig const& cfg)
{
    LayoutItem::Render(cfg);

    if (!m_src)
        return;

    auto w = obs_source_get_width(m_src);
    auto h = obs_source_get_height(m_src);
    vec2 scale { 1, 1 };
    int offset_x {}, offset_y {};

    if (m_toggle_stretch->isChecked()) {
        scale.x = m_inner_width / float(w);
        scale.y = m_inner_height / float(h);
    } else {
        GetScaleAndCenterPos(w, h, m_inner_width, m_inner_height, offset_x, offset_y, scale.x);
        scale.y = scale.x;
    }

    gs_matrix_push();
    gs_matrix_translate3f(offset_x, offset_y, 0);
    gs_matrix_scale3f(scale.x, scale.y, 1);
    obs_source_video_render(m_src);
    if (m_toggle_safe_borders->isChecked())
        RenderSafeMargins(w, h);
    gs_matrix_pop();

    if (m_vol_meter)
        m_vol_meter->render(cfg.scale);

    // Label has to be scaled and translated regardless of
    // source/scene size because sources can have sizes different than the base canvas
    if (m_toggle_label->isChecked() && m_label) {
        float label_scale = 1;
        int tmp_x {}, tmp_y {};
        auto lw = obs_source_get_width(m_label);
        auto lh = obs_source_get_height(m_label);

        if (lw == 0 || lh == 0)
            return;

        GetScaleAndCenterPos(cfg.canvas_width, cfg.canvas_height, m_inner_width, m_inner_height, tmp_x, tmp_y, label_scale);

        gs_matrix_push();
        // This is very convoluted, but I don't have a better way of doing this
        // Basically puts the label horziontally centered at the bottom of the source/scene with an offset from the bottom of 1.5 times the height of the label
        // The scale is the same as with the builtin multiview and uses the scale that a rectangle with the base canvas aspect ratio would need
        // this prevents the labels from getting too big/small (usually)
        gs_matrix_translate3f((m_inner_width - lw * label_scale) / 2, offset_y + h * scale.y - lh * label_scale * 1.5, 0);
        gs_matrix_scale3f(label_scale, label_scale, 1);
        DrawBox(lw, lh, labelColor);
        gs_matrix_translate3f(0, -(lh * 0.08), 0.0f);
        obs_source_video_render(m_label);
        gs_matrix_pop();
    }
}

void SourceItem::ContextMenu(QMenu& m)
{
    LayoutItem::ContextMenu(m);
    m.addAction(m_toggle_safe_borders);
    m.addAction(m_toggle_label);
    m.addAction(m_toggle_volume);
}

void SourceItem::MouseEvent(MouseData const& e, DurchblickItemConfig const& cfg)
{
    LayoutItem::MouseEvent(e, cfg);
    if (m_vol_meter) {
        if (e.buttons & Qt::LeftButton && m_mouse_over) {
            if (m_vol_meter->mouse_over(m_mouse_x, m_mouse_y)) {
                if (!m_dragging_volume) {
                    m_dragging_volume = true;
                    m_drag_start_x = m_mouse_x - m_vol_meter->get_x();
                    m_drag_start_y = m_mouse_y - m_vol_meter->get_y();
                }
            }

            if (m_dragging_volume) {
                m_vol_meter->set_pos(qBound(0, m_mouse_x - m_drag_start_x, m_width - m_vol_meter->get_width()), qBound(0, m_mouse_y - m_drag_start_y, m_height - m_vol_meter->get_height()));
            }
        } else {
            m_dragging_volume = false;
        }
    }
}
