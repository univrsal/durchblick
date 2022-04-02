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

#include "layout_config_dialog.hpp"
#include "layout.hpp"
#include "util.h"
#include <QHBoxLayout>
#include <QPushButton>

void LayoutConfigDialog::ok_clicked()
{
    m_layout->m_cols = m_cols->value();
    m_layout->m_rows = m_rows->value();

    m_layout->RefreshGrid();
    hide();
}

void LayoutConfigDialog::cancel_clicked()
{
    hide();
}

LayoutConfigDialog::LayoutConfigDialog(QWidget* parent, Layout* layout)
    : QDialog(parent)
    , m_layout(layout)
{
    m_vboxlayout = new QVBoxLayout(this);

    auto* hlayout = new QHBoxLayout(this);
    hlayout->addWidget(new QLabel(T_LABEL_GRID_SIZE, this));
    m_cols = new QSpinBox(this);
    m_rows = new QSpinBox(this);

    m_cols->setMaximum(16);
    m_cols->setMinimum(1);
    m_cols->setValue(layout->m_cols);
    m_rows->setMaximum(16);
    m_rows->setMinimum(1);
    m_rows->setValue(12);
    m_rows->setValue(layout->m_rows);

    hlayout->addWidget(m_cols);
    hlayout->addWidget(new QLabel("x", this));
    hlayout->addWidget(m_rows);
    hlayout->setContentsMargins(0, 0, 0, 0);
    m_vboxlayout->addLayout(hlayout);

    m_button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_vboxlayout->addWidget(m_button_box);
    setLayout(m_vboxlayout);
    connect(m_button_box->button(QDialogButtonBox::Ok), SIGNAL(pressed()), this, SLOT(ok_clicked()));
    connect(m_button_box->button(QDialogButtonBox::Cancel), SIGNAL(pressed()), this, SLOT(cancel_clicked()));
    setWindowTitle(T_LAYOUT_CONFIG);

    // Center dialog at mouse position
    auto p = QCursor::pos();
    move(p.x() - width() / 2, p.y() - height() / 2);
}
