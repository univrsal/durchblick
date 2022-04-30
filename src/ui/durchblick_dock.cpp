/*************************************************************************
 * This file is part of Durchblick
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

#include "durchblick_dock.hpp"
#include "../config.hpp"
#include "../util/display_helpers.hpp"
#include "../util/platform_util.hpp"
#include <QDesktopWidget>
#include <QIcon>
#include <QMainWindow>
#include <obs-module.h>
#include <obs.hpp>

void DurchblickDock::EscapeTriggered()
{
    hide();
}

void DurchblickDock::ResizeToContent()
{
    auto const& cfg = m_layout.Config();
    uint32_t targetCX = cfg.cell_width * m_layout.Columns();
    uint32_t targetCY = cfg.cell_height * m_layout.Rows();
    int x, y, newX, newY;
    float scale;

    QSize size = this->size();
    GetScaleAndCenterPos(targetCX, targetCY, size.width(), size.height(), x, y, scale);

    newX = size.width() - (x * 2);
    newY = size.height() - (y * 2);
    resize(newX, newY);
}

void DurchblickDock::AlwaysOnTopToggled(bool alwaysOnTop)
{
    SetIsAlwaysOnTop(alwaysOnTop);
}

void DurchblickDock::Resize(int cx, int cy)
{
    m_layout.Resize(m_fw, m_fh, cx, cy);
}

void DurchblickDock::ConvertToNormalWindow()
{
    QJsonObject config;
    Save(config);
    auto* window = new Durchblick;
    window->Load(config);
    window->setGeometry(geometry());
    window->show();
    Config::db = window;
    deleteLater();
}

void DurchblickDock::mouseMoveEvent(QMouseEvent* e)
{
    QWidget::mouseMoveEvent(e);
    m_layout.MouseMoved(e);
}

void DurchblickDock::mousePressEvent(QMouseEvent* e)
{
    QWidget::mousePressEvent(e);
    m_layout.MousePressed(e);
}

void DurchblickDock::mouseReleaseEvent(QMouseEvent* e)
{
    QWidget::mousePressEvent(e);
    m_layout.MouseReleased(e);
    if (e->button() == Qt::RightButton) {
        QMenu m(T_MENU_OPTION, this);
        auto* projectorMenu = new QMenu(T_FULLSCREEN);
        AddProjectorMenuMonitors(projectorMenu, this, SLOT(OpenFullScreenProjector()));
        m.addMenu(projectorMenu);

        m.addAction(T_RESIZE_WINDOW_CONTENT,
            this, SLOT(ResizeToContent()));

        auto* always_on_top = new QAction(T_ALWAYS_ON_TOP, this);
        always_on_top->setCheckable(true);
        always_on_top->setChecked(m_always_on_top);
        connect(always_on_top, &QAction::toggled, this, &DurchblickDock::AlwaysOnTopToggled);
        m.addAction(always_on_top);
        m.addAction(T_MENU_TO_NORMAL_WINDOW, this, SLOT(ConvertToNormalWindow()));

        m_layout.HandleContextMenu(e, m);
        m.exec(QCursor::pos());
    }
}

void DurchblickDock::mouseDoubleClickEvent(QMouseEvent* e)
{
    QWidget::mouseDoubleClickEvent(e);
    m_layout.MouseDoubleClicked(e);
}

void DurchblickDock::contextMenuEvent(QContextMenuEvent*)
{
}

void DurchblickDock::closeEvent(QCloseEvent* e)
{
    e->ignore();
    hide();
}

DurchblickDock::DurchblickDock(QWidget* widget)
    : OBSQTDockDisplay(widget, Qt::Window)
    , m_layout(this)
{
    setWindowTitle("Durchblick dock");
    setVisible(false);

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

    auto* close_action = new QAction(this);
    close_action->setShortcut(Qt::Key_Escape);
    connect(close_action, SIGNAL(triggered()), this, SLOT(EscapeTriggered()));
    addAction(close_action);

    auto addDrawCallback = [this]() {
        obs_display_add_draw_callback(GetDisplay(), RenderLayout, this);
        obs_display_set_background_color(GetDisplay(), 0x000000);
    };

    connect(this, &OBSQTDockDisplay::DisplayCreated, addDrawCallback);
    connect(this, &OBSQTDockDisplay::DisplayResized, this, &DurchblickDock::Resize);

    m_ready = true;
    show();

    // We need it here to allow keyboard input in X11 to listen to Escape
    activateWindow();
    Update();
    // Calculate initial layout values
    auto s = size() * devicePixelRatioF();
    Resize(s.width(), s.height());
    setMinimumSize({ 200, 112 });
}

DurchblickDock::~DurchblickDock()
{
    obs_display_remove_draw_callback(GetDisplay(), RenderLayout, this);
    m_ready = false;
    m_layout.DeleteLayout();
    deleteLater();
}

void DurchblickDock::RenderLayout(void* data, uint32_t cx, uint32_t cy)
{
    auto* w = (DurchblickDock*)data;
    if (!w->m_ready)
        return;
    w->m_layout.Render(w->m_fw, w->m_fh, cx, cy);
}

bool DurchblickDock::IsAlwaysOnTop() const
{
    return m_always_on_top;
}

void DurchblickDock::SetIsAlwaysOnTop(bool isAlwaysOnTop, bool reshow)
{
    m_always_on_top = isAlwaysOnTop;
    PlatformUtil::SetAlwaysOnTop(this, isAlwaysOnTop, reshow);
}

void DurchblickDock::Update()
{
    struct obs_video_info ovi;
    obs_get_video_info(&ovi);
    m_fw = ovi.base_width;
    m_fh = ovi.base_height;
    m_ratio = m_fw / m_fh;
}

void DurchblickDock::Save(QJsonObject& obj)
{
    auto main_window = static_cast<QMainWindow*>(obs_frontend_get_main_window());
    obj["is_dock"] = true;

    obj["geometry"] = QString(saveGeometry().toBase64());
    obj["dockarea"] = static_cast<QMainWindow*>(obs_frontend_get_main_window())->dockWidgetArea(this);
    obj["floating"] = isFloating();
    obj["visible"] = isVisible();
    obj["hide_from_display_capture"] = GetHideFromDisplayCapture();
    obj["hide_cursor"] = m_hide_cursor;
    obj["always_on_top"] = m_always_on_top;
    obj["corner_tl"] = main_window->corner(Qt::TopLeftCorner);
    obj["corner_br"] = main_window->corner(Qt::BottomRightCorner);
    obj["corner_tr"] = main_window->corner(Qt::TopRightCorner);
    obj["corner_bl"] = main_window->corner(Qt::BottomLeftCorner);
    obj["dockstate"] = main_window->saveState().toBase64().constData();
    m_layout.Save(obj);
}

void DurchblickDock::Load(QJsonObject const& obj)
{
    if (obj.isEmpty()) {
        berr("Layout object was null");
        m_layout.Clear();
        m_layout.CreateDefaultLayout();
        return;
    }

    SetHideCursor(obj["hide_cursor"].toBool(false));

    setVisible(obj["visible"].toBool(false));
    SetIsAlwaysOnTop(obj["always_on_top"].toBool(false), false);

    SetHideFromDisplayCapture(obj["hide_from_display_capture"].toBool(false));
    m_layout.Load(obj);

    auto main_window = static_cast<QMainWindow*>(obs_frontend_get_main_window());
    auto dockarea = static_cast<Qt::DockWidgetArea>(obj["dockarea"].toInt());

    //main_window->setCorner(Qt::TopLeftCorner, (Qt::DockWidgetArea)obj["corner_tl"].toInt());
    //main_window->setCorner(Qt::BottomRightCorner, (Qt::DockWidgetArea)obj["corner_br"].toInt());
    //main_window->setCorner(Qt::TopRightCorner, (Qt::DockWidgetArea)obj["corner_tr"].toInt());
    //main_window->setCorner(Qt::BottomLeftCorner, (Qt::DockWidgetArea)obj["corner_bl"].toInt());

    //if (dockarea != main_window->dockWidgetArea(this))
    //    main_window->addDockWidget(dockarea, this);

    //if (obj.contains("geometry") && obj["geometry"].isString()) {
    //    auto geo = QByteArray::fromBase64(obj["geometry"].toString().toUtf8().constData());
    //    restoreGeometry(geo);
    //}

    //if (obj.contains("dockstate") && obj["dockstate"].isString()) {
    //    QByteArray dockState = QByteArray::fromBase64(obj["dockstate"].toString().toUtf8().constData());
    //    main_window->restoreState(dockState);
    //}
    //setFloating(obj["floating"].toBool());
}

void DurchblickDock::SetHideFromDisplayCapture(bool hide_from_display_capture)
{
    // Yoinked from window-basic-main.cpp from obs-studio
    // technically (un)setting the window property should be enough but
    // that won't update the flags immediately
#ifdef _WIN32
    HWND hwnd = (HWND)winId();

    DWORD curAffinity;
    if (GetWindowDisplayAffinity(hwnd, &curAffinity)) {
        if (hide_from_display_capture && curAffinity != WDA_EXCLUDEFROMCAPTURE)
            SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE);
        else if (!hide_from_display_capture && curAffinity != WDA_NONE)
            SetWindowDisplayAffinity(hwnd, WDA_NONE);
    }
    windowHandle()->setProperty("isOBSProjectorWindow", !hide_from_display_capture);

    // Changing the display affinity causes some weird visual glitch which
    // fixes itself upon resizing the window so we do this right after changing the
    // flag
    resize(size().grownBy(QMargins(0, 0, 0, 1)));
    m_layout.RefreshGrid();
    resize(size().shrunkBy(QMargins(0, 0, 0, 1)));
#else
    UNUSED_PARAMETER(hide_from_display_capture);
#endif
}
