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
    if (!e->spontaneous()) {
        auto layouts = Config::LoadLayoutsForCurrentSceneCollection();
        db->GetLayout()->DeleteLayout();
        db->CreateDisplay(true);
        if (layouts.size() > 1) {
            db->Load(layouts[1].toObject());
        } else {
            db->GetLayout()->CreateDefaultLayout();
        }
        // Forces a grid refresh
        // just refreshing the grid doesn't seem to work
        resize(size().grownBy({ 1, 0, 0, 0 }));
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
