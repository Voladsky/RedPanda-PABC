#ifndef EXTERNALCOMPILERMANAGER_H
#define EXTERNALCOMPILERMANAGER_H

#include <QObject>
#include <QProcess>
#include <zmq.hpp>

class ExternalCompilerManager : public QObject
{
    Q_OBJECT
public:
    static ExternalCompilerManager& instance();

    ExternalCompilerManager(const ExternalCompilerManager&) = delete;
    ExternalCompilerManager& operator=(const ExternalCompilerManager&) = delete;

    void startCompiler();
    void killCompiler();
    void restartCompiler();
    void compile(const QString& filepath);

private:
    explicit ExternalCompilerManager(QObject *parent = nullptr);
    ~ExternalCompilerManager();
    void error(const QString& msg);
    void sendMessage(const std::string& message);

    QProcess* compilerProcess;
    zmq::context_t context;
    zmq::socket_t requester;
};

#endif // EXTERNALCOMPILERMANAGER_H
