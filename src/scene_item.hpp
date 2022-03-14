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
#include <QComboBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <obs.hpp>

class SceneItemWidget : public QWidget {
    Q_OBJECT
public:
    QComboBox* m_combo_box;
    SceneItemWidget(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        auto* l = new QFormLayout();
        setLayout(l);
        l->setContentsMargins(0, 0, 0, 0);
        m_combo_box = new QComboBox();
        m_combo_box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        l->addRow(T_SCENE_NAME, m_combo_box);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
};

class SceneItem : public SourceItem {
    Q_OBJECT
public:
    SceneItem(Layout* parent, int x, int y, int w = 1, int h = 1)
        : SourceItem(parent, x, y, w, h)
    {
    }
    ~SceneItem()
    {
        binfo("le");
    }

    QWidget* GetConfigWidget() override;
    void LoadConfigFromWidget(QWidget*) override;

    void Render(const Config& cfg) override;
};
