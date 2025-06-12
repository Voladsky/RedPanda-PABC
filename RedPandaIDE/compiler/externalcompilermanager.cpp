#include "externalcompilermanager.h"

#include <QDebug>
#include <QApplication>
#include <QMessageBox>
#include <QRegularExpression>
#include <QStandardPaths>

#include "mainwindow.h"
#include "settings.h"

ExternalCompilerManager& ExternalCompilerManager::instance()
{
    static ExternalCompilerManager instance(nullptr);
    return instance;
}

ExternalCompilerManager::ExternalCompilerManager(QObject* parent)
    : QObject(parent),
    context(1),
    requester(context, zmq::socket_type::req)
{
    compilerProcess = new QProcess(this);
    requester.connect("tcp://127.0.0.1:5555");
}

ExternalCompilerManager::~ExternalCompilerManager()
{
    compilerProcess->disconnect();
    compilerProcess->kill();
    compilerProcess->waitForFinished();
    requester.close();
    context.close();
}

QString ExternalCompilerManager::findPascalABCNET(const QString& exename)
{
    QStringList possiblePaths = {
        QCoreApplication::applicationDirPath() + "/../../" +
            QString("%1/share/%2/PascalABCNETLinux").arg(PREFIX).arg(APP_NAME),
        QCoreApplication::applicationDirPath() + "/../../../PascalABCNETLinux"
    };

    for (const QString& path : possiblePaths) {
        QFile file(path + "/" + exename);
        if (file.exists()) {
            return file.fileName();
        }
    }
    return QString();
}

void ExternalCompilerManager::startCompiler()
{
#ifdef Q_OS_WINDOWS
    QString path_to_pas = "D:\\Sci\\pascalabcnet-zmq\\bin\\pabcnetc.exe";
    compilerProcess->setProgram(path_to_pas);
    compilerProcess->setProcessChannelMode(QProcess::SeparateChannels);
    compilerProcess->setArguments(QStringList() << "/noconsole" << "commandmode");
#else
    QString path_to_mono = "mono";
    QString path_to_pas = findPascalABCNET("pabcnetc.exe");

    compilerProcess->setProgram(path_to_mono);
    compilerProcess->setProcessChannelMode(QProcess::SeparateChannels);
    compilerProcess->setArguments(QStringList()
                                  << path_to_pas
                                  << "/noconsole"
                                  << "commandmode");
#endif

    compilerProcess->start(QProcess::ReadWrite);
    if (!compilerProcess->waitForStarted()) {
        qDebug() << "Failed to start process!";
    } else {
        qDebug() << "PABCNETC Process started successfully.";
    }
}

void ExternalCompilerManager::killCompiler()
{
    compilerProcess->kill();
}

void ExternalCompilerManager::restartCompiler()
{
    killCompiler();
    compilerProcess->waitForFinished();
    startCompiler();
}

void ExternalCompilerManager::sendMessage(const std::string& message)
{
    try {
        zmq::message_t msg(message.size());
        memcpy(msg.data(), message.c_str(), message.size());
        requester.send(msg, zmq::send_flags::none);

        zmq::pollitem_t items[] = {{static_cast<void*>(requester), 0, ZMQ_POLLIN, 0}};
        zmq::poll(items, 1, 15000);

        if (items[0].revents & ZMQ_POLLIN) {
            zmq::message_t reply;
            if (requester.recv(reply)) {
                QString replyMessage = QString::fromStdString(
                    std::string(static_cast<char*>(reply.data()), reply.size()));

                pMainWindow->logToolsOutput(replyMessage);
                if (replyMessage.startsWith("100")) {
                    pMainWindow->logToolsOutput("COMPILED SUCCESSFULLY!");
                } else {
                    error(replyMessage);
                }
            } else {
                QMessageBox::warning(pMainWindow, "Error",
                                     "Failed to receive response.");
            }
        } else {
            QMessageBox::warning(pMainWindow, "Error",
                                 "No response received within the timeout period.");
        }
    } catch (const std::exception& e) {
        QMessageBox::critical(pMainWindow, "Error",
                              QString("Communication error: %1").arg(e.what()));
    }
}

void ExternalCompilerManager::error(const QString& msg)
{
    QRegularExpression regex(R"(^\[([^\]]+)\]\[(\d+),(\d+)\]\s+(.*?):\s+(.*)(\[1\])*)");
    QRegularExpressionMatchIterator i = regex.globalMatch(msg);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        if (match.hasMatch()) {
            PCompileIssue issue = std::make_shared<CompileIssue>();
            issue->line = match.captured(2).toInt();
            issue->column = match.captured(3).toInt();
            issue->filename = match.captured(4);
            issue->description = match.captured(5);
            issue->type = CompileIssueType::Error;
            pMainWindow->onCompileIssue(issue);
        }
    }
}

void ExternalCompilerManager::compile(const QString& filepath)
{
    std::string message = "215#5#" + filepath.toStdString();
    sendMessage(message);
    sendMessage("210");
    pMainWindow->onCompileFinished(filepath, true);
}

void ExternalCompilerManager::scheduleRestart(int msecs) {
    timer.setInterval(msecs);
    connect(&timer, &QTimer::timeout, this, &ExternalCompilerManager::restartCompiler);
    timer.start();
}
