#include "new_item_dialog.hpp"
#include "layout.hpp"
#include "registry.hpp"
#include "util.h"
#include <QGroupBox>
#include <QPushButton>
#include <QSizePolicy>
#include <QSpacerItem>

void NewItemDialog::ok_clicked()
{
    auto index = m_select_type->currentIndex();
    if (index >= 0 && index < Registry::ItemRegistry::Entries.size()) {
        m_layout->AddWidget(Registry::ItemRegistry::Entries[index], m_last_config_widget);
    }
    hide();
}

void NewItemDialog::cancel_clicked()
{
    hide();
}

void NewItemDialog::entry_selected(int index)
{
    auto* Item = Registry::ItemRegistry::Entries[index].construct(nullptr, 0, 0, 0, 0, nullptr);
    auto* Widget = Item->GetConfigWidget();
    if (Widget) {
        Widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        if (m_last_config_widget) {
            m_config_layout->removeWidget(m_last_config_widget);
            m_last_config_widget->hide();
            m_last_config_widget->deleteLater();
        }
        m_config_layout->addWidget(Widget);
        m_last_config_widget = Widget;
    }
    delete Item;
}

NewItemDialog::NewItemDialog(QWidget* parent, Layout* layout)
    : QDialog(parent)
    , m_layout(layout)
{
    m_config_layout = new QVBoxLayout(this);
    m_vboxlayout = new QVBoxLayout(this);
    m_vboxlayout->addWidget(new QLabel(T_SELECT_TYPE, this));
    m_button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_select_type = new QComboBox(this);
    m_config_panel = new QGroupBox(T_WIDGET_SETTINGS, this);
    m_config_panel->setLayout(m_config_layout);
    //    m_config_layout->setContentsMargins(0, 0, 0, 0);

    for (auto const& E : qAsConst(Registry::ItemRegistry::Entries))
        m_select_type->addItem(E.name);
    entry_selected(0);
    m_vboxlayout->addWidget(m_select_type);
    m_vboxlayout->addWidget(m_config_panel);
    m_vboxlayout->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
    m_vboxlayout->addWidget(m_button_box);
    setLayout(m_vboxlayout);
    connect(m_button_box->button(QDialogButtonBox::Ok), SIGNAL(pressed()), this, SLOT(ok_clicked()));
    connect(m_button_box->button(QDialogButtonBox::Cancel), SIGNAL(pressed()), this, SLOT(cancel_clicked()));
    connect(m_select_type, SIGNAL(currentIndexChanged(int)), this, SLOT(entry_selected(int)));
    setWindowTitle(T_SELECT_TYPE_DIALOG);
    resize(minimumSizeHint());
    // Center dialog at mouse position
    auto p = QCursor::pos();
    move(p.x() - width() / 2, p.y() - height() / 2);
}
