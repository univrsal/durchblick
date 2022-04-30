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
#include "../layout.hpp"
#include "durchblick.hpp"
#include "qt_dock_display.hpp"
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

class DurchblickDock : public OBSQTDockDisplay
    , public IDurchblick {
    Q_OBJECT

    bool m_hide_cursor { false };
    bool m_always_on_top { false };

public:
    bool m_ready { false };
    Layout m_layout;
    uint32_t m_fw {}, m_fh {}; // Base canvas width and height
    float m_ratio { 16.f / 9.f };
private slots:
    void EscapeTriggered();
    void ResizeToContent();
    void AlwaysOnTopToggled(bool alwaysOnTop);

    void Resize(int cx, int cy);
    void ConvertToNormalWindow();

protected:
    virtual void mouseMoveEvent(QMouseEvent*) override;
    virtual void mousePressEvent(QMouseEvent*) override;
    virtual void mouseReleaseEvent(QMouseEvent*) override;
    virtual void mouseDoubleClickEvent(QMouseEvent*) override;
    virtual void contextMenuEvent(QContextMenuEvent*) override;

    virtual void closeEvent(QCloseEvent*) override;

protected:
    //    void dragEnterEvent(QDragEnterEvent *event) override;
    //    void dragMoveEvent(QDragMoveEvent *event) override;
    //    void dragLeaveEvent(QDragLeaveEvent *event) override; // TODO: dragging cells?
    //    void dropEvent(QDropEvent* event) override;

public:
    DurchblickDock(QWidget* widget = nullptr);
    ~DurchblickDock();

    static void RenderLayout(void* data, uint32_t cx, uint32_t cy);
    bool IsAlwaysOnTop() const;
    void SetIsAlwaysOnTop(bool isAlwaysOnTop, bool reshow = true);
    void Update();

    void Save(QJsonObject& obj) override;
    /// Will either load fromt he JSON object or create the default layout
    void Load(QJsonObject const& obj) override;

    void SetHideFromDisplayCapture(bool hide_from_display_capture);

    bool GetHideFromDisplayCapture()
    {
        return !windowHandle()->property("isOBSProjectorWindow").toBool();
    }

    void SetHideCursor(bool hide)
    {
        m_hide_cursor = hide;
        if (hide)
            setCursor(Qt::BlankCursor);
        else
            setCursor(Qt::ArrowCursor);
    }

    bool GetIsCursorHidden() const { return m_hide_cursor; }

    Layout* GetLayout() { return &m_layout; }

    QWidget* AsWidget() override { return this; }
    bool IsDock() const override { return true; }
};
