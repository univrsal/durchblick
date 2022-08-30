#include "durchblick_dock.hpp"
#include "config.hpp"
#include "durchblick.hpp"
#include <QShowEvent>
#include <QVBoxLayout>

void DurchblickDock::closeEvent(QCloseEvent* e)
{
    e->accept();
    db->OnClose();
    Config::Save();
    db->GetLayout()->DeleteLayout();
    db->DeleteDisplay();
    hide();
}

void DurchblickDock::showEvent(QShowEvent* e)
{
    QWidget::showEvent(e);
    auto layouts = Config::LoadLayoutsForCurrentSceneCollection();

    if (layouts.size() > 1) {
        if (layouts[1].toObject().isEmpty())
            db->GetLayout()->CreateDefaultLayout();
        else
            db->Load(layouts[1].toObject());
    }
}

DurchblickDock::DurchblickDock(QWidget* parent)
    : QDockWidget(parent)
    , db(new Durchblick(this))
{
    setFeatures(DockWidgetMovable | DockWidgetFloatable);
    setWindowTitle("Durchblick");
    setObjectName("DurchblickDock");
    setFloating(true);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(db);

    auto* dockWidgetContents = new QWidget;
    dockWidgetContents->setLayout(mainLayout);
    setWidget(dockWidgetContents);
}

DurchblickDock::~DurchblickDock()
{
}
