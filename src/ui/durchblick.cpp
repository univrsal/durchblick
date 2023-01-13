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
#include "../config.hpp"
#include "../util/platform_util.hpp"
#include "obs.hpp"
#include <QApplication>
#include <QIcon>
#include <QWindow>
#include <obs-module.h>

#ifdef _WIN32
#    include "../util/windows_helper.hpp"
#    include <Windows.h>
#endif

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

void Durchblick::EscapeTriggered()
{
    hide();
}

void Durchblick::OpenFullScreenProjector()
{
    if (!isFullScreen())
        m_previous_geometry = geometry();

    int monitor = sender()->property("monitor").toInt();
    SetMonitor(monitor);

    if (monitor < 0) // Windowed
        setWindowTitle("Durchblick");
    else
        setWindowTitle("Durchblick (" + T_FULLSCREEN + ")");
}

void Durchblick::OpenWindowedProjector()
{
    showFullScreen();
    showNormal();
    setCursor(Qt::ArrowCursor);
    if (m_previous_geometry.isNull()) {
        resize(480, 270);
        move(30, 30);
    } else {
        setGeometry(m_previous_geometry);
    }

    m_current_monitor = -1;
    setWindowTitle("Durchblick");
    m_screen = nullptr;
}

void Durchblick::ResizeToContent()
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

void Durchblick::AlwaysOnTopToggled(bool alwaysOnTop)
{
    SetIsAlwaysOnTop(alwaysOnTop);
}

void Durchblick::ScreenRemoved(QScreen* screen_)
{
    if (m_current_monitor < 0 || !m_screen)
        return;

    if (m_screen == screen_)
        EscapeTriggered();
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
    if (e->button() == Qt::RightButton) {
        QMenu m(T_MENU_OPTION, this);
        auto* projectorMenu = new QMenu(T_FULLSCREEN);

        if (!m_layout.IsLocked()) {
            AddProjectorMenuMonitors(projectorMenu, this, SLOT(OpenFullScreenProjector()));
            m.addMenu(projectorMenu);

            if (m_current_monitor > -1) {
                m.addAction(T_WINDOWED, this, SLOT(OpenWindowedProjector()));
            } else if (!this->isMaximized()) {
                m.addAction(T_RESIZE_WINDOW_CONTENT,
                    this, SLOT(ResizeToContent()));
            }

            auto* always_on_top = new QAction(T_ALWAYS_ON_TOP, this);
            always_on_top->setCheckable(true);
            always_on_top->setChecked(m_always_on_top);
            connect(always_on_top, &QAction::toggled, this, &Durchblick::AlwaysOnTopToggled);
            m.addAction(always_on_top);
        }

        m_layout.HandleContextMenu(e, m);
        m.exec(QCursor::pos());
    }
}

void Durchblick::mouseDoubleClickEvent(QMouseEvent* e)
{
    QWidget::mouseDoubleClickEvent(e);
    m_layout.MouseDoubleClicked(e);
}

void Durchblick::contextMenuEvent(QContextMenuEvent*)
{
}

void Durchblick::closeEvent(QCloseEvent* e)
{
    e->accept();
    OnClose();
    Config::Save();
    m_layout.DeleteLayout();
    hide();
    DeleteDisplay();
}

void Durchblick::showEvent(QShowEvent* e)
{
    QWidget::showEvent(e);
    if (m_saved_state == WindowState::Maximized)
        setWindowState(windowState() | Qt::WindowMaximized);
    else if (m_current_monitor >= 0)
        SetMonitor(m_current_monitor);
}

Durchblick::Durchblick(QWidget* widget)
    : OBSQTDisplay(widget, Qt::Window)
    , m_layout(this)
{
    setWindowTitle("Durchblick");
    setVisible(false);

#ifdef __APPLE__
    setWindowIcon(
        QIcon::fromTheme("obs", QIcon(":/res/images/obs_256x256.png")));
#else
    setWindowIcon(QIcon::fromTheme("obs", QIcon(":/res/images/obs.png")));
#endif

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

    connect(this, &OBSQTDisplay::DisplayCreated, addDrawCallback);
    connect(qApp, &QGuiApplication::screenRemoved, this,
        &Durchblick::ScreenRemoved);
    connect(this, &OBSQTDisplay::DisplayResized, this, &Durchblick::Resize);

    m_ready = true;
    show();

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
    m_screen = nullptr;
    m_ready = false;
    m_layout.DeleteLayout();
    deleteLater();
}

void Durchblick::OnClose()
{
    m_saved_state = GetWindowState();
}

void Durchblick::RenderLayout(void* data, uint32_t cx, uint32_t cy)
{
    auto* w = (Durchblick*)data;
    if (!w->m_ready || !w->isVisible())
        return;
    w->m_layout.Render(w->m_fw, w->m_fh, cx, cy);
}

void Durchblick::SetMonitor(int monitor)
{
    if (monitor < 0)
        return;
    m_current_monitor = qMin(monitor, QGuiApplication::screens().length() - 1);
    m_screen = QGuiApplication::screens().at(m_current_monitor);
    setWindowState(Qt::WindowActive);
    setGeometry(m_screen->geometry());
    showFullScreen();
    // TODO: Option for hiding cursor?
}

bool Durchblick::IsAlwaysOnTop() const
{
    return m_always_on_top;
}

void Durchblick::SetIsAlwaysOnTop(bool isAlwaysOnTop, bool reshow)
{
    m_always_on_top = isAlwaysOnTop;
    PlatformUtil::SetAlwaysOnTop(this, isAlwaysOnTop, reshow);
}

void Durchblick::Update()
{
    struct obs_video_info ovi;
    obs_get_video_info(&ovi);
    m_fw = ovi.base_width;
    m_fh = ovi.base_height;
    m_ratio = m_fw / m_fh;
    m_has_size = m_fw > 0 && m_fh > 0;
}

void Durchblick::Save(QJsonObject& obj)
{
    if (isVisible()) {
        obj["monitor"] = m_current_monitor;
        QJsonObject geo;
        geo["x"] = geometry().x();
        geo["y"] = geometry().y();
        geo["w"] = geometry().width();
        geo["h"] = geometry().height();
        obj["geometry"] = geo;
        obj["visible"] = isVisible();
        obj["state"] = GetWindowState();
        obj["hide_from_display_capture"] = GetHideFromDisplayCapture();
        obj["hide_cursor"] = m_hide_cursor;
        obj["always_on_top"] = m_always_on_top;
        m_layout.Save(obj);
        m_cached_layout = obj;
    } else {
        // When the window is hidden we delete the layout
        // so that we don't hold onto source references
        // that also means that we don't have a layout to save, so we just use the last saved layout
        // as it can't really change while the window isn't visible
        obj = m_cached_layout;
        obj["visible"] = false;
    }
}

void Durchblick::Load(QJsonObject const& obj)
{
    if (obj.isEmpty()) {
        berr("Layout object was null");
        m_layout.Clear();
        m_layout.CreateDefaultLayout();
        return;
    }
    m_cached_layout = obj;
    m_saved_state = (WindowState)obj["state"].toInt(WindowState::None);

    // Restore geometry if this view wasn't in fullscreen
    if (m_current_monitor < 0 && obj.contains("geometry") && obj["geometry"].isObject()) {
        auto geo = obj["geometry"].toObject();
        if (geo.contains("x") && geo.contains("y") && geo.contains("w") && geo.contains("h")) {
            QRect geometry(geo["x"].toInt(), geo["y"].toInt(), geo["w"].toInt(480), geo["h"].toInt(270));
            setGeometry(geometry);
        }
    }

    if (m_saved_state == WindowState::Maximized)
        setWindowState(windowState() | Qt::WindowMaximized);

    if (obj.contains("monitor"))
        SetMonitor(obj["monitor"].toInt(-1));

    SetHideCursor(obj["hide_cursor"].toBool(false));

    setVisible(obj["visible"].toBool(false));
    SetIsAlwaysOnTop(obj["always_on_top"].toBool(false), false);

    SetHideFromDisplayCapture(obj["hide_from_display_capture"].toBool(false));
    m_layout.Load(obj);
}

void Durchblick::SetHideFromDisplayCapture(bool hide_from_display_capture)
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
