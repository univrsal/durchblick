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
#include <QScreen>
#include <QVBoxLayout>
#if _WIN32
#    include <obs-frontend-api.h>
#else
#    include <obs/obs-frontend-api.h>
#endif

class Durchblick : public OBSQTDisplay {
    Q_OBJECT
    obs_frontend_event_cb m_frontend_cb {};

public:
    bool m_ready { false };
    QScreen* m_screen { nullptr };
    Layout m_layout;
    uint32_t m_fw {}, m_fh {};
    float m_ratio { 16 / 9. };
private slots:
    void EscapeTriggered();
    // void OpenFullScreenProjector();
    // void ResizeToContent();
    // void OpenWindowedProjector();
    // void AlwaysOnTopToggled(bool alwaysOnTop);
    void ScreenRemoved(QScreen* screen_);
    void Resize(int cx, int cy);

protected:
    virtual void mouseMoveEvent(QMouseEvent*) override;
    virtual void mousePressEvent(QMouseEvent*) override;
    virtual void mouseReleaseEvent(QMouseEvent*) override;
    virtual void contextMenuEvent(QContextMenuEvent*) override;

protected:
    //    void dragEnterEvent(QDragEnterEvent *event) override;
    //    void dragMoveEvent(QDragMoveEvent *event) override;
    //    void dragLeaveEvent(QDragLeaveEvent *event) override; // TODO: dragging cells?
    //    void dropEvent(QDropEvent* event) override;

public:
    Durchblick(QWidget* widget = nullptr);
    ~Durchblick();

    static void RenderLayout(void* data, uint32_t cx, uint32_t cy);

    //    int GetMonitor();
    //    void RenameProjector(QString oldName, QString newName);
    //    void SetHideCursor();

    //    bool IsAlwaysOnTop() const;
    //    bool IsAlwaysOnTopOverridden() const;
    //    void SetIsAlwaysOnTop(bool isAlwaysOnTop, bool isOverridden);
    void Update();

    void Save(QJsonObject& obj);
    void Load(QJsonObject const& obj);

    Layout* GetLayout() { return &m_layout; }
};
