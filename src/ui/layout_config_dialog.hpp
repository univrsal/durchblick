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

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

class Layout;
class Durchblick;

class LayoutConfigDialog : public QDialog {
    Q_OBJECT
    QVBoxLayout* m_vboxlayout {};
    QDialogButtonBox* m_button_box {};
    QSpinBox *m_cols {}, *m_rows {};
    QCheckBox *m_hide_from_display_capture {}, *m_hide_cursor {};
    Layout* m_layout {};
    Durchblick* m_durchblick {};
private slots:
    void OKClicked();
    void CancelClicked();

public:
    LayoutConfigDialog(Durchblick* parent, Layout* layout);
};
