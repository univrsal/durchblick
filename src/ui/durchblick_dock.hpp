#pragma once
#include <QDockWidget>
#include <QTextEdit>
#include <obs-frontend-api.h>

class Durchblick;

class DurchblickDock : public QDockWidget {
    Q_OBJECT

private:
    Durchblick* db;

protected:
    void closeEvent(QCloseEvent*) override;
    void showEvent(QShowEvent*) override;

public:
    DurchblickDock(QWidget* parent = nullptr);
    ~DurchblickDock();

    Durchblick* GetDurchblick() { return db; }
};