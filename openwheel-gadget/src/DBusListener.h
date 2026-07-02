#pragma once
#include <QObject>
#include <QDBusServiceWatcher>

class DBusListener : public QObject {
    Q_OBJECT
public:
    explicit DBusListener(QObject *parent = nullptr);

    bool isDaemonConnected() const;

signals:
    void rotated(int direction);
    void pressChanged(bool pressed);
    void daemonConnectedChanged(bool connected);

private slots:
    void onRotate(int value);
    void onPress(int value);

private:
    QDBusServiceWatcher m_watcher;
    bool m_daemonConnected = false;
};
