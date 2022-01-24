#include "new_item_dialog.hpp"
#include "layout.hpp"
#include "registry.hpp"
#include "util.h"
#include <QPushButton>

void NewItemDialog::ok_clicked()
{
    auto index = m_select_type->currentIndex();
    if (index >= 0 && index < Registry::ItemRegistry::Entries.size()) {
        m_layout->AddWidget(Registry::ItemRegistry::Entries[index]);
    }
    hide();
}

void NewItemDialog::cancel_clicked()
{
    hide();
}

NewItemDialog::NewItemDialog(QWidget* parent, Layout* layout)
    : QDialog(parent)
    , m_layout(layout)
{
    m_vboxlayout = new QVBoxLayout(this);
    m_vboxlayout->addWidget(new QLabel(T_SELECT_TYPE, this));
    m_button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_select_type = new QComboBox(this);

    for (auto const& E : qAsConst(Registry::ItemRegistry::Entries))
        m_select_type->addItem(E.name);
    m_vboxlayout->addWidget(m_select_type);
    m_vboxlayout->addWidget(m_button_box);
    setLayout(m_vboxlayout);
    connect(m_button_box->button(QDialogButtonBox::Ok), SIGNAL(pressed()), this, SLOT(ok_clicked()));
    connect(m_button_box->button(QDialogButtonBox::Cancel), SIGNAL(pressed()), this, SLOT(ok_clicked()));
    setWindowTitle(T_SELECT_TYPE_DIALOG);
}
