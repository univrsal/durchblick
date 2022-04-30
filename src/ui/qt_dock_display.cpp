/*************************************************************************
 * https://github.com/obsproject/obs-studio/blob/master/UI/qt-display.hpp
 *************************************************************************/

#include "qt_dock_display.hpp"
#include "../util/display_helpers.hpp"
#include <QGuiApplication>
#include <QResizeEvent>
#include <QScreen>
#include <QShowEvent>
#include <QWindow>
#include <obs-config.h>

OBSQTDockDisplay::OBSQTDockDisplay(QWidget* parent, Qt::WindowFlags flags)
    : QDockWidget(parent, flags)
{
    setAttribute(Qt::WA_StaticContents);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_DontCreateNativeAncestors);
    setAttribute(Qt::WA_NativeWindow);

    auto windowVisible = [this](bool visible) {
        if (!visible) {
#ifdef ENABLE_WAYLAND
            if (obs_get_nix_platform() == OBS_NIX_PLATFORM_WAYLAND)
                display = nullptr;
#endif
            return;
        }

        if (!display) {
            CreateDisplay();
        } else {
            QSize size = GetPixelSize(this);
            obs_display_resize(display, size.width(),
                size.height());
        }
    };

    auto screenChanged = [this](QScreen*) {
        CreateDisplay();

        QSize size = GetPixelSize(this);
        obs_display_resize(display, size.width(), size.height());
    };

    connect(windowHandle(), &QWindow::visibleChanged, windowVisible);
    connect(windowHandle(), &QWindow::screenChanged, screenChanged);

#ifdef ENABLE_WAYLAND
    if (obs_get_nix_platform() == OBS_NIX_PLATFORM_WAYLAND)
        windowHandle()->installEventFilter(
            new SurfaceEventFilter(this));
#endif
}

QColor OBSQTDockDisplay::GetDisplayBackgroundColor() const
{
    return rgba_to_color(backgroundColor);
}

void OBSQTDockDisplay::SetDisplayBackgroundColor(QColor const& color)
{
    uint32_t newBackgroundColor = (uint32_t)color_to_int(color);

    if (newBackgroundColor != backgroundColor) {
        backgroundColor = newBackgroundColor;
        UpdateDisplayBackgroundColor();
    }
}

void OBSQTDockDisplay::UpdateDisplayBackgroundColor()
{
    obs_display_set_background_color(display, backgroundColor);
}

void OBSQTDockDisplay::CreateDisplay(bool force)
{
    if (display)
        return;

    if (!windowHandle()->isExposed() && !force)
        return;

    QSize size = GetPixelSize(this);

    gs_init_data info = {};
    info.cx = size.width();
    info.cy = size.height();
    info.format = GS_BGRA;
    info.zsformat = GS_ZS_NONE;

    if (!QTToGSWindow(windowHandle(), info.window))
        return;

    display = obs_display_create(&info, backgroundColor);

    emit DisplayCreated(this);
}

void OBSQTDockDisplay::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    CreateDisplay();

    QSize size = GetPixelSize(this);
    if (isVisible() && display) {
        obs_display_resize(display, size.width(), size.height());
    }

    emit DisplayResized(size.width(), size.height());
}

void OBSQTDockDisplay::paintEvent(QPaintEvent* event)
{
    CreateDisplay();

    QWidget::paintEvent(event);
}

QPaintEngine* OBSQTDockDisplay::paintEngine() const
{
    return nullptr;
}
