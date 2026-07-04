#pragma once
#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include "TrayController.h"

class TrayIcon : public QObject {
    Q_OBJECT
public:
    explicit TrayIcon(TrayController *controller, QObject *parent = nullptr);
    void show();
    void setDaemonConnected(bool connected);

private:
    TrayController *m_controller;
    QMenu m_menu;
    QAction *m_enabledAction;
    QAction *m_quitAction;
    QSystemTrayIcon m_icon;
};
