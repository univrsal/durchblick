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

#include "source_item.hpp"
#include "util.h"
#include <QApplication>
#include <QHBoxLayout>
#include <QRadioButton>
#include <obs.hpp>

class PreviewProgramItemWidget : public QWidget {
    Q_OBJECT
public:
    QRadioButton *m_preview, *m_program;
    PreviewProgramItemWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        auto* l = new QHBoxLayout();
        setLayout(l);
        l->setContentsMargins(0, 0, 0, 0);
        m_preview = new QRadioButton(QApplication::translate("", T_PREVIEW));
        m_program = new QRadioButton(QApplication::translate("", T_PROGRAM));
        m_preview->setChecked(true);
        l->addWidget(m_preview);
        l->addWidget(m_program);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
};

class PreviewProgramItem : public SourceItem {
    Q_OBJECT
    bool m_program { false };

public:
    PreviewProgramItem(Layout* parent, int x, int y, int w = 1, int h = 1)
        : SourceItem(parent, x, y, w, h)
    {
    }

    ~PreviewProgramItem() = default;
    void SetIsProgram(bool b) { m_program = b; }
    QWidget* GetConfigWidget() override;
    void LoadConfigFromWidget(QWidget*) override;
    void CreateLabel();
    void Render(DurchblickItemConfig const& cfg) override;

    void WriteToJson(QJsonObject& Obj) override;
    void ReadFromJson(QJsonObject const& Obj) override;
};
