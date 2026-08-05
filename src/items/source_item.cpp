#include "source_item.hpp"
#include "../layout.hpp"
#include "../util/display_helpers.hpp"
#include "../util/performance_stats.hpp"
#include <QApplication>
#include <QMainWindow>
#include <cmath>
#include <map>
#include <mutex>
#include <obs-frontend-api.h>
#include <tuple>
#include <unordered_map>
#include <util/util.hpp>

namespace {
using LabelKey = std::tuple<std::string, size_t, int>;
std::mutex label_cache_mutex;
std::map<LabelKey, OBSSource> label_cache;
std::mutex source_count_mutex;
std::unordered_map<obs_source_t*, size_t> source_counts;

void AddSourceInstance(obs_source_t* source)
{
    if (!source)
        return;
    std::lock_guard<std::mutex> lock(source_count_mutex);
    ++source_counts[source];
}

void RemoveSourceInstance(obs_source_t* source)
{
    if (!source)
        return;
    std::lock_guard<std::mutex> lock(source_count_mutex);
    auto it = source_counts.find(source);
    if (it != source_counts.end() && --it->second == 0)
        source_counts.erase(it);
}

using RenderCacheKey = std::tuple<obs_source_t*, uint32_t, uint32_t,
    gs_color_format>;
struct RenderCacheEntry {
    gs_texrender_t* target {};
    uint64_t frame_time {};
    uint64_t last_used {};
};
std::map<RenderCacheKey, RenderCacheEntry> source_render_cache;
uint64_t render_cache_last_cleanup {};

gs_texture_t* RenderSourceCached(obs_source_t* source, uint32_t source_width,
    uint32_t source_height, uint32_t target_width, uint32_t target_height)
{
    const gs_color_space color_space = gs_get_color_space();
    const gs_color_format color_format = gs_get_format_from_space(color_space);
    RenderCacheKey key { source, target_width, target_height, color_format };
    const uint64_t frame_time = obs_get_video_frame_time();
    if (frame_time - render_cache_last_cleanup > 5000000000ULL) {
        for (auto it = source_render_cache.begin();
            it != source_render_cache.end();) {
            if (frame_time - it->second.last_used > 10000000000ULL) {
                if (it->second.target)
                    gs_texrender_destroy(it->second.target);
                it = source_render_cache.erase(it);
            } else {
                ++it;
            }
        }
        render_cache_last_cleanup = frame_time;
    }

    auto& entry = source_render_cache[key];
    entry.last_used = frame_time;
    if (entry.target && entry.frame_time == frame_time) {
        PerformanceStats::SourceCacheHit();
        return gs_texrender_get_texture(entry.target);
    }
    PerformanceStats::SourceCacheMiss();

    if (!entry.target)
        entry.target = gs_texrender_create(color_format, GS_ZS_NONE);
    gs_texrender_reset(entry.target);
    if (!gs_texrender_begin_with_color_space(entry.target, target_width,
            target_height, color_space))
        return nullptr;

    vec4 clear_color;
    vec4_zero(&clear_color);
    gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);
    gs_matrix_push();
    gs_matrix_identity();
    StartRegion(0, 0, target_width, target_height, 0.0f,
        float(source_width), 0.0f, float(source_height));
    obs_source_video_render(source);
    EndRegion();
    gs_matrix_pop();
    gs_texrender_end(entry.target);
    entry.frame_time = frame_time;
    return gs_texrender_get_texture(entry.target);
}
}

OBSSource CreateLabel(char const* name, size_t h, float scale)
{
    const int font_size = int(h / 9.81) * scale;
    LabelKey key { name ? name : "", h, font_size };
    std::lock_guard<std::mutex> lock(label_cache_mutex);
    if (auto it = label_cache.find(key); it != label_cache.end())
        return it->second;

    OBSDataAutoRelease settings = obs_data_create();
    OBSDataAutoRelease font = obs_data_create();
    std::string text = " " + std::get<0>(key) + " ";
#if defined(_WIN32)
    obs_data_set_string(font, "face", "Arial");
    const char* text_source_id = "text_gdiplus";
#elif defined(__APPLE__)
    obs_data_set_string(font, "face", "Helvetica");
    const char* text_source_id = "text_ft2_source";
#else
    obs_data_set_string(font, "face", "Monospace");
    const char* text_source_id = "text_ft2_source";
#endif
    obs_data_set_int(font, "flags", 1);
    obs_data_set_int(font, "size", font_size);
    obs_data_set_obj(settings, "font", font);
    obs_data_set_string(settings, "text", text.c_str());
    obs_data_set_bool(settings, "outline", false);

    OBSSourceAutoRelease source = obs_source_create_private(text_source_id, name, settings);
    OBSSource shared = source.Get();
    label_cache.emplace(std::move(key), shared);
    return shared;
}

void SourceItem::VolumeToggled(bool state)
{
    if (state && m_src) {
        auto h = obs_source_get_height(m_src);
        m_vol_meter = std::make_unique<MixerMeter>(m_src, m_volume_meter_x, m_volume_meter_y, h / 2);
    } else {
        if (m_vol_meter) {
            m_volume_meter_x = m_vol_meter->GetX();
            m_volume_meter_y = m_vol_meter->GetY();
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
    MixerMeter::Init();
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
    MixerMeter::Deinit();
    obs_enter_graphics();
    {
        std::lock_guard<std::mutex> lock(label_cache_mutex);
        label_cache.clear();
        for (auto& [key, entry] : source_render_cache) {
            UNUSED_PARAMETER(key);
            if (entry.target)
                gs_texrender_destroy(entry.target);
        }
        source_render_cache.clear();
    }
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
    RemoveSourceInstance(window->m_src);
    window->m_src = placeholder_source;
    AddSourceInstance(window->m_src);
    if (window->m_vol_meter)
        window->m_vol_meter->SetSource(placeholder_source);
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
    connect(m_toggle_stretch, &QAction::toggled, this,
        [this] { m_transform_dirty = true; });
    connect(m_toggle_volume, SIGNAL(toggled(bool)), this, SLOT(VolumeToggled(bool)));
}

SourceItem::~SourceItem()
{
    RemoveSourceInstance(m_src);
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
            m_vol_meter = std::make_unique<MixerMeter>(src.Get(), 10, 10, int(h * m_volume_meter_height));
        }
        SetSource(src);
    }
}

void SourceItem::SetSource(obs_source_t* src)
{
    if (m_src) {
        RemoveSourceInstance(m_src);
        obs_source_dec_showing(m_src);
    }

    m_src = src;
    AddSourceInstance(m_src);
    m_transform_dirty = true;
    if (m_src) {
        if (m_vol_meter)
            m_vol_meter->SetSource(src);
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

bool SourceItem::HasDuplicateRenderSource(obs_source_t* source)
{
    std::lock_guard<std::mutex> lock(source_count_mutex);
    const auto it = source_counts.find(source);
    return it != source_counts.end() && it->second > 1;
}

void SourceItem::Update(DurchblickItemConfig const& cfg)
{
    LayoutItem::Update(cfg);
    m_transform_dirty = true;
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
        m_vol_meter = std::make_unique<MixerMeter>(src.Get(), m_volume_meter_x, m_volume_meter_y, int(h * m_volume_meter_height));
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
        Obj["volume_meter_x"] = m_vol_meter->GetX();
        Obj["volume_meter_y"] = m_vol_meter->GetY();
    } else {
        Obj["volume_meter_x"] = m_volume_meter_x;
        Obj["volume_meter_y"] = m_volume_meter_y;
    }
}

static const uint32_t labelColor = 0xD91F1F1F;

void SourceItem::Render(DurchblickItemConfig const& cfg)
{
    LayoutItem::Render(cfg);

    if (!m_src)
        return;

    const int w = obs_source_get_width(m_src);
    const int h = obs_source_get_height(m_src);
    if (w <= 0 || h <= 0)
        return;

    if (m_transform_dirty || w != m_source_width || h != m_source_height) {
        m_source_width = w;
        m_source_height = h;
        m_source_offset_x = 0;
        m_source_offset_y = 0;
        if (m_toggle_stretch->isChecked()) {
            m_scale.x = m_inner_width / float(w);
            m_scale.y = m_inner_height / float(h);
        } else {
            GetScaleAndCenterPos(w, h, m_inner_width, m_inner_height,
                m_source_offset_x, m_source_offset_y, m_scale.x);
            m_scale.y = m_scale.x;
        }
        if (m_label) {
            m_label_width = obs_source_get_width(m_label);
            m_label_height = obs_source_get_height(m_label);
            int unused_x {}, unused_y {};
            GetScaleAndCenterPos(cfg.canvas_width, cfg.canvas_height,
                m_inner_width, m_inner_height, unused_x, unused_y,
                m_label_scale);
        }
        m_transform_dirty = false;
    }

    gs_matrix_push();
    gs_matrix_translate3f(m_source_offset_x, m_source_offset_y, 0);
    gs_matrix_scale3f(m_scale.x, m_scale.y, 1);
    if (m_use_render_cache) {
        const uint32_t target_width = qMax(
            1U, uint32_t(std::ceil(w * m_scale.x * cfg.scale)));
        const uint32_t target_height = qMax(
            1U, uint32_t(std::ceil(h * m_scale.y * cfg.scale)));
        gs_texture_t* texture = RenderSourceCached(m_src, w, h,
            target_width, target_height);
        if (texture) {
            gs_effect_t* effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
            gs_eparam_t* image = gs_effect_get_param_by_name(effect, "image");
            gs_effect_set_texture(image, texture);
            while (gs_effect_loop(effect, "Draw"))
                gs_draw_sprite(texture, 0, w, h);
        }
    } else {
        obs_source_video_render(m_src);
    }
    if (m_toggle_safe_borders->isChecked())
        RenderSafeMargins(w, h);
    gs_matrix_pop();

    if (m_vol_meter && obs_source_active(m_src))
        m_vol_meter->Render(cfg.scale, m_scale.x, m_scale.y);

    // Label has to be scaled and translated regardless of
    // source/scene size because sources can have sizes different than the base canvas
    if (m_toggle_label->isChecked() && m_label) {
        const auto lw = m_label_width;
        const auto lh = m_label_height;

        if (lw == 0 || lh == 0)
            return;

        gs_matrix_push();
        // This is very convoluted, but I don't have a better way of doing this
        // Basically puts the label horziontally centered at the bottom of the source/scene with an offset from the bottom of 1.5 times the height of the label
        // The scale is the same as with the builtin multiview and uses the scale that a rectangle with the base canvas aspect ratio would need
        // this prevents the labels from getting too big/small (usually)
        gs_matrix_translate3f((m_inner_width - lw * m_label_scale) / 2, m_source_offset_y + h * m_scale.y - lh * m_label_scale * 1.5, 0);
        gs_matrix_scale3f(m_label_scale, m_label_scale, 1);
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
    if (EnableVolumeMeter())
        m.addAction(m_toggle_volume);
}

void SourceItem::MouseEvent(MouseData const& e, DurchblickItemConfig const& cfg)
{
    LayoutItem::MouseEvent(e, cfg);
    if (m_vol_meter) {
        if (e.buttons & Qt::LeftButton && m_mouse_over) {
            if (m_vol_meter->MouseOver(m_mouse_x, m_mouse_y)) {
                if (!m_dragging_volume) {
                    m_dragging_volume = true;
                    m_drag_start_x = m_mouse_x - m_vol_meter->GetX();
                    m_drag_start_y = m_mouse_y - m_vol_meter->GetY();
                }
            }

            if (m_dragging_volume) {
                auto x = qBound(0, m_mouse_x - m_drag_start_x, qMax(int(m_width - m_vol_meter->GetWidth() * m_scale.x), 1));
                auto y = qBound(0, m_mouse_y - m_drag_start_y, qMax(int(m_height - m_vol_meter->GetHeight() * m_scale.y), 1));
                m_vol_meter->SetPos(x, y);
            }
        } else {
            m_dragging_volume = false;
        }
    }
}
