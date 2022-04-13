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
#pragma once
#include "layout.hpp"
#include "qt_display.hpp"
#include <QRect>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>

#if _WIN32
#    include <obs-frontend-api.h>
#else
#    include <obs/obs-frontend-api.h>
#endif

class Durchblick : public OBSQTDisplay {
    Q_OBJECT
    QTimer m_hover_refresh {};

    enum WindowState {
        None,
        Maximized,
    } m_saved_state {};

    WindowState GetWindowState() const
    {
        if (isMaximized())
            return WindowState::Maximized;
        return WindowState::None;
    }

public:
    QRect m_previous_geometry;
    bool m_ready { false };
    QScreen* m_screen { nullptr };
    int m_current_monitor { -1 };
    Layout m_layout;
    uint32_t m_fw {}, m_fh {}; // Base canvas width and height
    float m_ratio { 16 / 9. };
private slots:
    void EscapeTriggered();
    void OpenFullScreenProjector();
    void OpenWindowedProjector();
    void ResizeToContent();
    // void AlwaysOnTopToggled(bool alwaysOnTop);
    void ScreenRemoved(QScreen* screen_);
    void Resize(int cx, int cy);
    void UpdateHover();

protected:
    virtual void mouseMoveEvent(QMouseEvent*) override;
    virtual void mousePressEvent(QMouseEvent*) override;
    virtual void mouseReleaseEvent(QMouseEvent*) override;
    virtual void mouseDoubleClickEvent(QMouseEvent*) override;
    virtual void contextMenuEvent(QContextMenuEvent*) override;

    virtual void closeEvent(QCloseEvent*) override;
    virtual void showEvent(QShowEvent*) override;

protected:
    //    void dragEnterEvent(QDragEnterEvent *event) override;
    //    void dragMoveEvent(QDragMoveEvent *event) override;
    //    void dragLeaveEvent(QDragLeaveEvent *event) override; // TODO: dragging cells?
    //    void dropEvent(QDropEvent* event) override;

public:
    Durchblick(QWidget* widget = nullptr);
    ~Durchblick();

    static void RenderLayout(void* data, uint32_t cx, uint32_t cy);

    void SetMonitor(int monitor);

    //    int GetMonitor();
    //    void RenameProjector(QString oldName, QString newName);
    //    void SetHideCursor();

    //    bool IsAlwaysOnTop() const;
    //    bool IsAlwaysOnTopOverridden() const;
    //    void SetIsAlwaysOnTop(bool isAlwaysOnTop, bool isOverridden);
    void Update();

    void Save(QJsonObject& obj);
    void Load(QJsonObject const& obj);

    void SetHideFromDisplayCapture(bool hide_from_display_capture);

    bool GetHideFromDisplayCapture()
    {
        return !windowHandle()->property("isOBSProjectorWindow").toBool();
    }

    Layout* GetLayout() { return &m_layout; }
};
