#pragma once
#include <QObject>

class TrayController : public QObject {
    Q_OBJECT
public:
    explicit TrayController(QObject *parent = nullptr);
    bool isEnabled() const;

public slots:
    void toggleEnabled();

signals:
    void enabledChanged(bool enabled);
    void quitRequested();

private:
    bool m_enabled = true;
};
