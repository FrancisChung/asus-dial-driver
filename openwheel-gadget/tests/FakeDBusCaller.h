#pragma once
#include "DBusCaller.h"

class FakeDBusCaller : public DBusCaller {
public:
    QDBusMessage call(DBusBus bus, const QString &service, const QString &path,
                       const QString &interface, const QString &method,
                       const QVariantList &args = {}) override
    {
        lastBus = bus;
        lastService = service;
        lastPath = path;
        lastInterface = interface;
        lastMethod = method;
        lastArgs = args;
        callCount++;

        QDBusMessage request = QDBusMessage::createMethodCall(service, path, interface, method);
        return request.createReply(nextReplyArgs);
    }

    DBusBus lastBus = DBusBus::Session;
    QString lastService;
    QString lastPath;
    QString lastInterface;
    QString lastMethod;
    QVariantList lastArgs;
    int callCount = 0;
    QVariantList nextReplyArgs;
};
