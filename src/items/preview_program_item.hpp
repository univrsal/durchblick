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

#include "../util/util.h"
#include "source_item.hpp"
#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <obs.hpp>

class PreviewProgramItemWidget : public QWidget {
    Q_OBJECT
public:
    QRadioButton *m_preview, *m_program;
    QDoubleSpinBox* m_font_size;
    PreviewProgramItemWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        auto* form = new QFormLayout(this);
        auto* l = new QHBoxLayout();
        l->setContentsMargins(0, 0, 0, 0);
        m_preview = new QRadioButton(T_PREVIEW, this);
        m_program = new QRadioButton(T_PROGRAM, this);
        m_preview->setChecked(true);
        l->addWidget(m_preview);
        l->addWidget(m_program);
        form->addRow("", l);
        form->setContentsMargins(0, 0, 0, 0);

        m_font_size = new QDoubleSpinBox(this);
        m_font_size->setMinimum(1);
        m_font_size->setValue(100);
        m_font_size->setMaximum(500);
        m_font_size->setSuffix("%");
        m_font_size->setDecimals(0);
        form->addRow(T_FONT_SIZE, m_font_size);
        setLayout(form);
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
    bool EnableVolumeMeter() const override { return false; }
};
