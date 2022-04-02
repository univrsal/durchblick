/*************************************************************************
 * This file is part of durchblick
 * git.vrsal.xyz/alex/durchblick
 * Copyright 2022 univrsal <uni@vrsal.xyz>.
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
#include "obs.hpp"
#include <QWindow>
#include <QApplication>
#include <QIcon>
#include <obs-module.h>

void Durchblick::EscapeTriggered()
{
}

void Durchblick::ScreenRemoved(QScreen* screen_)
{
    //    if (GetMonitor() < 0 || !m_screen)
    //        return;

    //    if (m_screen == screen_)
    //        EscapeTriggered();
}

void Durchblick::Resize(int cx, int cy)
{
    m_layout.Resize(m_fw, m_fh, cx, cy);
}

void Durchblick::mouseMoveEvent(QMouseEvent* e)
{
    QWidget::mouseMoveEvent(e);
    m_layout.MouseMoved(e);
}

void Durchblick::mousePressEvent(QMouseEvent* e)
{
    QWidget::mousePressEvent(e);
    m_layout.MousePressed(e);
}

void Durchblick::mouseReleaseEvent(QMouseEvent* e)
{
    QWidget::mousePressEvent(e);
    m_layout.MouseReleased(e);
    if (e->button() == Qt::RightButton)
        m_layout.HandleContextMenu(nullptr);
}

void Durchblick::mouseDoubleClickEvent(QMouseEvent* e)
{
    QWidget::mouseDoubleClickEvent(e);
    m_layout.MouseDoubleClicked(e);
}

void Durchblick::contextMenuEvent(QContextMenuEvent* e)
{
}

void Durchblick::closeEvent(QCloseEvent* e)
{
    e->ignore();
    hide();
}

Durchblick::Durchblick(QWidget* widget)
    : OBSQTDisplay(widget, Qt::Window)
    , m_layout(this)
{
    setWindowTitle("Durchblick");

    // Mark the window as a projector so SetDisplayAffinity
    // can skip it
    windowHandle()->setProperty("isOBSProjectorWindow", true);

#ifdef __APPLE__
    setWindowIcon(
        QIcon::fromTheme("obs", QIcon(":/res/images/obs_256x256.png")));
#else
    setWindowIcon(QIcon::fromTheme("obs", QIcon(":/res/images/obs.png")));
#endif
    setAttribute(Qt::WA_DeleteOnClose, false);
    // disable application quit when last window closed
    setAttribute(Qt::WA_QuitOnClose, false);
    setMouseTracking(true);
    // qApp->IncrementSleephibition();

    auto addDrawCallback = [this]() {
        obs_display_add_draw_callback(GetDisplay(), RenderLayout, this);
        obs_display_set_background_color(GetDisplay(), 0x000000);
    };

    connect(this, &OBSQTDisplay::DisplayCreated, addDrawCallback);
    connect(qApp, &QGuiApplication::screenRemoved, this,
        &Durchblick::ScreenRemoved);
    connect(this, &OBSQTDisplay::DisplayResized, this, &Durchblick::Resize);
    m_ready = true;
    show();

    m_frontend_cb = [](enum obs_frontend_event event, void* private_data) {
        if (event == OBS_FRONTEND_EVENT_EXIT) {
            auto* db = static_cast<Durchblick*>(private_data);
            db->m_ready = false;
            db->m_layout.DeleteLayout();
            db->deleteLater();
        }
    };
    obs_frontend_add_event_callback(m_frontend_cb, this);

    // We need it here to allow keyboard input in X11 to listen to Escape
    activateWindow();
    Update();
    // Calculate initial layout values
    auto s = size() * devicePixelRatioF();
    Resize(s.width(), s.height());
}

Durchblick::~Durchblick()
{
    obs_display_remove_draw_callback(GetDisplay(), RenderLayout, this);
    obs_frontend_remove_event_callback(m_frontend_cb, this);
    m_screen = nullptr;
    deleteLater();
}

void Durchblick::RenderLayout(void* data, uint32_t cx, uint32_t cy)
{
    auto* w = (Durchblick*)data;
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

void Durchblick::Save(QJsonObject& obj)
{
    m_layout.Save(obj);
}

void Durchblick::Load(const QJsonObject& obj)
{
    m_layout.Load(obj);
}
