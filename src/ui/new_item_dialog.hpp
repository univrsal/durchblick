/*************************************************************************
 * This file is part of durchblick
 * git.vrsal.xyz/alex/durchblick
 * Copyright 2023 univrsal <uni@vrsal.xyz>.
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
class Durchblick;

class NewItemDialog : public QDialog {
    Q_OBJECT
    QComboBox* m_select_type;
    QVBoxLayout *m_vboxlayout, *m_config_layout;
    QDialogButtonBox* m_button_box;
    QWidget *m_config_panel, *m_last_config_widget = nullptr;
    Layout* m_layout;
private slots:
    void OKClicked();
    void CancelClicked();
    void EntrySelected(int);

public:
    NewItemDialog(Durchblick* parent, Layout* layout);
};
