#ifndef INTELLISENSEMANAGER_H
#define INTELLISENSEMANAGER_H

#include <QObject>
#include <QProcess>
#include <QUrl>
#include <QHash>

#include <zmq.hpp>
#include "qsynedit/types.h"
#include "editor.h"

class IntelliSenseManager : public QObject
{
    Q_OBJECT
public:
    static IntelliSenseManager& instance();

    IntelliSenseManager(const IntelliSenseManager&) = delete;
    IntelliSenseManager& operator=(const IntelliSenseManager&) = delete;

    void startIntelli();
    void killIntelli();
    void restartIntelli();
    void connectEvents();
    void didOpen(const QString& filename, Editor* editor);
    void didClose(const QString& filename);
    void didChange(const QString& filename, const QString& fulltext);
    void didSave(const QString& filename);
    QStringList callIntelli(const QSynedit::BufferCoord& pos,
                            const QString& filename,
                            const QString& request_type);

private:
    explicit IntelliSenseManager(QObject* parent = nullptr);
    void initializeLSP(const QString& filename);
    QString sendMessage(const std::string& message);
    QHash<QString, int> documentVersions;
    int requestId = 0;
    QProcess* intelliProcess = nullptr;
    zmq::context_t context;
    zmq::socket_t requester;
};

#endif // INTELLISENSEMANAGER_H
