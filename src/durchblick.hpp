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
#pragma once
#include "qt_display.hpp"
#include <QScreen>
#include <QVBoxLayout>
#include "layout.hpp"

class Durchblick : public OBSQTDisplay {
    Q_OBJECT
public:
    bool m_ready { false };
    QScreen *m_screen {nullptr};
    Layout m_layout;
    uint32_t m_fw{}, m_fh{};
    float m_ratio{16/9.};
private slots:
    void EscapeTriggered();
    //void OpenFullScreenProjector();
    //void ResizeToContent();
    //void OpenWindowedProjector();
    //void AlwaysOnTopToggled(bool alwaysOnTop);
    void ScreenRemoved(QScreen *screen_);

public:
    Durchblick(QWidget *widget);
    ~Durchblick();

    static void RenderLayout(void *data, uint32_t cx, uint32_t cy);

    int GetMonitor();
    void RenameProjector(QString oldName, QString newName);
    void SetHideCursor();

    bool IsAlwaysOnTop() const;
    bool IsAlwaysOnTopOverridden() const;
    void SetIsAlwaysOnTop(bool isAlwaysOnTop, bool isOverridden);
    void Update();
};

class Test : public QWidget {
    Q_OBJECT

    QVBoxLayout *mainLayout{};
    Durchblick *preview{};
public:
    OBSSource source;
    Test(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        mainLayout = new QVBoxLayout(this);
        setObjectName(QStringLiteral("contextContainer"));
        setLayout(mainLayout);
        preview = new Durchblick(this);
        preview->setObjectName(QStringLiteral("preview"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(
            preview->sizePolicy().hasHeightForWidth());
        preview->setSizePolicy(sizePolicy1);
        preview->setMinimumSize(QSize(24, 24));

        preview->setMouseTracking(true);
        preview->setFocusPolicy(Qt::StrongFocus);
        mainLayout->addWidget(preview);
    }

    ~Test()
    {
        mainLayout->removeWidget(preview);
        preview->deleteLater();
    }
};
