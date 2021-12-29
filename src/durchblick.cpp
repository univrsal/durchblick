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

#include "durchblick.hpp"
#include <obs-frontend-api.h>
#include <obs-module.h>
#include <QIcon>
#include <QApplication>
#include "obs.hpp"

void Durchblick::EscapeTriggered()
{
}

void Durchblick::ScreenRemoved(QScreen *screen_)
{
    if (GetMonitor() < 0 || !m_screen)
        return;

    if (m_screen == screen_)
        EscapeTriggered();
}

Durchblick::Durchblick(QWidget *widget)
    : OBSQTDisplay(widget, Qt::Window)
{
#ifdef __APPLE__
    setWindowIcon(
        QIcon::fromTheme("obs", QIcon(":/res/images/obs_256x256.png")));
#else
    setWindowIcon(QIcon::fromTheme("obs", QIcon(":/res/images/obs.png")));
#endif
    setAttribute(Qt::WA_DeleteOnClose, true);
    //disable application quit when last window closed
    setAttribute(Qt::WA_QuitOnClose, false);

    //qApp->IncrementSleephibition();

    //installEventFilter(CreateShortcutFilter());

    auto addDrawCallback = [this]() {
        obs_display_add_draw_callback(GetDisplay(), RenderLayout, this);
        obs_display_set_background_color(GetDisplay(), 0x000000);
    };

    connect(this, &OBSQTDisplay::DisplayCreated, addDrawCallback);
    connect(qApp, &QGuiApplication::screenRemoved, this,
        &Durchblick::ScreenRemoved);

    m_ready = true;
    show();

    // We need it here to allow keyboard input in X11 to listen to Escape
    activateWindow();
    Update();
}

Durchblick::~Durchblick()
{
    obs_display_remove_draw_callback(GetDisplay(), RenderLayout, this);
    m_screen = nullptr;
}

void Durchblick::RenderLayout(void *data, uint32_t cx, uint32_t cy)
{
    auto *w = (Durchblick *)data;
    if (!w->m_ready)
        return;
    w->m_layout.Render(w->m_fw, w->m_fh, cx, cy);
}

void Durchblick::Update()
{
    struct obs_video_info ovi;
    obs_get_video_info(&ovi);
    m_fw = ovi.base_width;
    m_fh = ovi.base_height;
    m_ratio = m_fw / m_fh;
}
