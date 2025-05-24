#ifndef INTELLISENSEMANAGER_H
#define INTELLISENSEMANAGER_H

#include <QObject>
#include <QProcess>
#include <QUrl>
#include "qsynedit/types.h"
#include <zmq.hpp>
#include <qhash.h>
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
    QStringList callIntelli(const QSynedit::BufferCoord& pos, const QString& filename, const QString request_type);
    void connectEvents();
    void didOpen(const QString& filename, Editor* editor);
    void didClose(const QString& filename);
    void didChange(const QString& filename, const QString& fulltext);
    void didSave(const QString& filename);
private:
    explicit IntelliSenseManager(QObject *parent = nullptr);
    //~IntelliSenseManager();
    void initializeLSP(const QString& filename);
    QString sendMessage(const std::string& message);
    QHash<QString, int> docVersions;
    int requestId;
    QHash<QString, int> documentVersions; // Tracks document versions
    QProcess* compilerProcess;
    zmq::context_t context;
    zmq::socket_t requester;
};

#endif // INTELLISENSEMANAGER_H
