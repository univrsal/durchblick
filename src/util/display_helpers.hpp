/******************************************************************************
    Copyright (C) 2014 by Hugh Bailey <obs.jim@gmail.com>
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#pragma once

#include "util.h"
#if defined(_WIN32)
#    include "windows_helper.hpp"
#endif

#if !defined(_WIN32) && !defined(__APPLE__)
#    include <obs-nix-platform.h>
#    include <qpa/qplatformnativeinterface.h>
#endif

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QScreen>
#include <QTextStream>
#include <QWindow>
#include <graphics/matrix4.h>
#include <obs-module.h>

#define OUTLINE_COLOR 0xFFD0D0D0
#define LINE_LENGTH 0.1f

// Rec. ITU-R BT.1848-1 / EBU R 95
#define ACTION_SAFE_PERCENT 0.035f       // 3.5%
#define GRAPHICS_SAFE_PERCENT 0.05f      // 5.0%
#define FOURBYTHREE_SAFE_PERCENT 0.1625f // 16.25%

static inline void InitSafeAreas(gs_vertbuffer_t** actionSafeMargin,
    gs_vertbuffer_t** graphicsSafeMargin,
    gs_vertbuffer_t** fourByThreeSafeMargin,
    gs_vertbuffer_t** leftLine,
    gs_vertbuffer_t** topLine,
    gs_vertbuffer_t** rightLine)
{
    obs_enter_graphics();

    // All essential action should be placed inside this area
    gs_render_start(true);
    gs_vertex2f(ACTION_SAFE_PERCENT, ACTION_SAFE_PERCENT);
    gs_vertex2f(ACTION_SAFE_PERCENT, 1 - ACTION_SAFE_PERCENT);
    gs_vertex2f(1 - ACTION_SAFE_PERCENT, 1 - ACTION_SAFE_PERCENT);
    gs_vertex2f(1 - ACTION_SAFE_PERCENT, ACTION_SAFE_PERCENT);
    gs_vertex2f(ACTION_SAFE_PERCENT, ACTION_SAFE_PERCENT);
    *actionSafeMargin = gs_render_save();

    // All graphics should be placed inside this area
    gs_render_start(true);
    gs_vertex2f(GRAPHICS_SAFE_PERCENT, GRAPHICS_SAFE_PERCENT);
    gs_vertex2f(GRAPHICS_SAFE_PERCENT, 1 - GRAPHICS_SAFE_PERCENT);
    gs_vertex2f(1 - GRAPHICS_SAFE_PERCENT, 1 - GRAPHICS_SAFE_PERCENT);
    gs_vertex2f(1 - GRAPHICS_SAFE_PERCENT, GRAPHICS_SAFE_PERCENT);
    gs_vertex2f(GRAPHICS_SAFE_PERCENT, GRAPHICS_SAFE_PERCENT);
    *graphicsSafeMargin = gs_render_save();

    // 4:3 safe area for widescreen
    gs_render_start(true);
    gs_vertex2f(FOURBYTHREE_SAFE_PERCENT, GRAPHICS_SAFE_PERCENT);
    gs_vertex2f(1 - FOURBYTHREE_SAFE_PERCENT, GRAPHICS_SAFE_PERCENT);
    gs_vertex2f(1 - FOURBYTHREE_SAFE_PERCENT, 1 - GRAPHICS_SAFE_PERCENT);
    gs_vertex2f(FOURBYTHREE_SAFE_PERCENT, 1 - GRAPHICS_SAFE_PERCENT);
    gs_vertex2f(FOURBYTHREE_SAFE_PERCENT, GRAPHICS_SAFE_PERCENT);
    *fourByThreeSafeMargin = gs_render_save();

    gs_render_start(true);
    gs_vertex2f(0.0f, 0.5f);
    gs_vertex2f(LINE_LENGTH, 0.5f);
    *leftLine = gs_render_save();

    gs_render_start(true);
    gs_vertex2f(0.5f, 0.0f);
    gs_vertex2f(0.5f, LINE_LENGTH);
    *topLine = gs_render_save();

    gs_render_start(true);
    gs_vertex2f(1.0f, 0.5f);
    gs_vertex2f(1 - LINE_LENGTH, 0.5f);
    *rightLine = gs_render_save();

    obs_leave_graphics();
}

static inline void RenderSafeAreas(gs_vertbuffer_t* vb, int cx, int cy)
{
    if (!vb)
        return;

    matrix4 transform;
    matrix4_identity(&transform);
    transform.x.x = cx;
    transform.y.y = cy;

    gs_load_vertexbuffer(vb);

    gs_matrix_push();
    gs_matrix_mul(&transform);

    gs_effect_t* solid = obs_get_base_effect(OBS_EFFECT_SOLID);
    gs_eparam_t* color = gs_effect_get_param_by_name(solid, "color");

    gs_effect_set_color(color, OUTLINE_COLOR);
    while (gs_effect_loop(solid, "Solid"))
        gs_draw(GS_LINESTRIP, 0, 0);

    gs_matrix_pop();
}

static inline bool QTToGSWindow(QWindow* window, gs_window& gswindow)
{
    bool success = true;

#ifdef _WIN32
    gswindow.hwnd = (HWND)window->winId();
#elif __APPLE__
    gswindow.view = (id)window->winId();
#else
    switch (obs_get_nix_platform()) {
    case OBS_NIX_PLATFORM_X11_GLX:
    case OBS_NIX_PLATFORM_X11_EGL:
        gswindow.id = window->winId();
        gswindow.display = obs_get_nix_platform_display();
        break;
#    ifdef ENABLE_WAYLAND
    case OBS_NIX_PLATFORM_WAYLAND:
        QPlatformNativeInterface* native = QGuiApplication::platformNativeInterface();
        gswindow.display = native->nativeResourceForWindow("surface", window);
        success = gswindow.display != nullptr;
        break;
#    endif
    }
#endif
    return success;
}

static inline QSize GetPixelSize(QWidget* widget)
{
    return widget->size() * widget->devicePixelRatioF();
}

static inline long long color_to_int(QColor const& color)
{
    auto shift = [&](unsigned val, int shift) {
        return ((val & 0xff) << shift);
    };

    return shift(color.red(), 0) | shift(color.green(), 8) | shift(color.blue(), 16) | shift(color.alpha(), 24);
}

static inline QColor rgba_to_color(uint32_t rgba)
{
    return QColor::fromRgb(rgba & 0xFF, (rgba >> 8) & 0xFF,
        (rgba >> 16) & 0xFF, (rgba >> 24) & 0xFF);
}

// Snagged from window-basic-main.cpp from obs-studio
static void AddProjectorMenuMonitors(QMenu* parent, QObject* target, const char* slot)
{
    QAction* action;
    QList<QScreen*> screens = QGuiApplication::screens();
    for (int i = 0; i < screens.size(); i++) {
        QScreen* screen = screens[i];
        QRect screenGeometry = screen->geometry();
        qreal ratio = screen->devicePixelRatio();
        QString name = "";
#ifdef _WIN32
        QTextStream fullname(&name);
        fullname << GetMonitorName(screen->name());
        fullname << " (";
        fullname << (i + 1);
        fullname << ")";
#elif defined(__APPLE__)
        name = screen->name();
#else
        name = screen->model().simplified();

        if (name.length() > 1 && name.endsWith("-"))
            name.chop(1);
#endif
        name = name.simplified();

        if (name.length() == 0) {
            name = QString("%1 %2").arg(T_DISPLAY, QString::number(i + 1));
        }

        QString str = QString("%1: %2x%3 @ %4,%5")
                          .arg(name,
                              QString::number(screenGeometry.width() * ratio),
                              QString::number(screenGeometry.height() * ratio),
                              QString::number(screenGeometry.x()),
                              QString::number(screenGeometry.y()));

        action = parent->addAction(str, target, slot);
        action->setProperty("monitor", i);
    }
}

static inline bool window_pos_valid(QRect rect)
{
    for (const auto& screen : QGuiApplication::screens()) {
        if (screen->availableGeometry().intersects(rect))
            return true;
    }
    return false;
}

#ifdef ENABLE_WAYLAND

class SurfaceEventFilter : public QObject {
    OBSQTDockDisplay* display;
    int mTimerId;

public:
    SurfaceEventFilter(OBSQTDockDisplay* src)
        : display(src)
        , mTimerId(0)
    {
    }

protected:
    bool eventFilter(QObject* obj, QEvent* event) override
    {
        bool result = QObject::eventFilter(obj, event);
        QPlatformSurfaceEvent* surfaceEvent;

        switch (event->type()) {
        case QEvent::PlatformSurface:
            surfaceEvent = static_cast<QPlatformSurfaceEvent*>(event);
            if (surfaceEvent->surfaceEventType() != QPlatformSurfaceEvent::SurfaceCreated)
                return result;

            if (display->windowHandle()->isExposed())
                createOBSDisplay();
            else
                mTimerId = startTimer(67); // Arbitrary
            break;
        case QEvent::Expose:
            createOBSDisplay();
            break;
        default:
            break;
        }

        return result;
    }

    void timerEvent(QTimerEvent*) { createOBSDisplay(true); }

private:
    void createOBSDisplay(bool force = false)
    {
        display->CreateDisplay(force);
        if (mTimerId > 0) {
            killTimer(mTimerId);
            mTimerId = 0;
        }
    }
};

#endif