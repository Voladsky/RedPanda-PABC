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

    // Delete copy constructor and assignment operator
    IntelliSenseManager(const IntelliSenseManager&) = delete;
    IntelliSenseManager& operator=(const IntelliSenseManager&) = delete;

    // IntelliSense operations
    void startIntelli();
    void killIntelli();
    void restartIntelli();

    // Document operations
    void connectEvents();
    void didOpen(const QString& filename, Editor* editor);
    void didClose(const QString& filename);
    void didChange(const QString& filename, const QString& fulltext);
    void didSave(const QString& filename);

    // Code analysis
    QStringList callIntelli(const QSynedit::BufferCoord& pos,
                            const QString& filename,
                            const QString& request_type);

private:
    explicit IntelliSenseManager(QObject* parent = nullptr);

    // LSP communication
    void initializeLSP(const QString& filename);
    QString sendMessage(const std::string& message);

    // Member variables
    QHash<QString, int> documentVersions; // Tracks document versions
    int requestId = 0;
    QProcess* compilerProcess = nullptr;

    // ZMQ communication
    zmq::context_t context;
    zmq::socket_t requester;
};

#endif // INTELLISENSEMANAGER_H
