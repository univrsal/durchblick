/*************************************************************************
 * This file is part of input-overlay
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

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>

class Layout;

class NewItemDialog : public QDialog {
    Q_OBJECT
    QComboBox* m_select_type;
    QVBoxLayout* m_vboxlayout;
    QDialogButtonBox* m_button_box;
    Layout* m_layout;
private slots:
    void ok_clicked();
    void cancel_clicked();

public:
    NewItemDialog(QWidget* parent, Layout* layout);
};
