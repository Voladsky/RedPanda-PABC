#include "externalcompilermanager.h"
#include <QDebug>
#include <QApplication>
#include "mainwindow.h"
#include <QMessageBox>
#include <QRegularExpression>
#include <QStandardPaths>
#include <settings.h>

ExternalCompilerManager& ExternalCompilerManager::instance()
{
    static ExternalCompilerManager instance(nullptr);
    return instance;
}

ExternalCompilerManager::ExternalCompilerManager(QObject *parent)
    : QObject(parent),
    context(1),
    requester(context, zmq::socket_type::req)
{
    compilerProcess = new QProcess(this);
    requester.connect("tcp://127.0.0.1:5555");
}

void handlePascalError() {
    QMessageBox::critical(pMainWindow, "", "ERRROR!");
}

QString findPascalABCNET(const QString& exename) {
    qDebug() << QCoreApplication::applicationDirPath();
    QStringList possiblePaths = {
        QCoreApplication::applicationDirPath() + "/../../" + QString("%1/share/%2/PascalABCNETLinux").arg(PREFIX).arg(APP_NAME),
        QCoreApplication::applicationDirPath() + "/../../../PascalABCNETLinux"
    };
    for (const QString& path : possiblePaths) {
        QFile file(path + "/" + exename);
        qDebug() << file;
        if (file.exists()) {
            return file.fileName();
        }
    }
    return QString();
}

void ExternalCompilerManager::startCompiler() {
#ifdef Q_OS_WINDOWS
    QString path_to_pas = "D:\\Sci\\pascalabcnet-zmq\\bin\\pabcnetc.exe";
    //QString path_to_pas = pSettings->dirs().appDir() + "/../PascalABCNETLinux/pabcnetc.exe"; // Path to the C# executable
    compilerProcess->setProgram(path_to_pas);
    compilerProcess->setProcessChannelMode(QProcess::SeparateChannels);
    compilerProcess->setArguments(QStringList() << "/noconsole" << "commandmode");
#else
    QString path_to_mono = "mono";
    compilerProcess->setProgram(path_to_mono);
    compilerProcess->setProcessChannelMode(QProcess::SeparateChannels);
    QString path_to_pas = findPascalABCNET("pabcnetc.exe");
    qDebug() << path_to_pas;
    compilerProcess->setArguments(QStringList() << path_to_pas.toStdString().c_str() << "/noconsole" << "commandmode");
#endif
    // Connect signals for output and errors
    //connect(compilerProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::handlePascalOutput);
    connect(compilerProcess, QOverload<QProcess::ProcessError>::of(&QProcess::errorOccurred), this, handlePascalError);

    // Start the process in ReadWrite mode
    compilerProcess->start(QProcess::ReadWrite);
    if (!compilerProcess->waitForStarted()) {
        qDebug() << "Failed to start process!";
    } else {
        qDebug() << "PABCNETC Process started successfully.";
    }
}

void ExternalCompilerManager::killCompiler() {
    compilerProcess->kill();
}

void ExternalCompilerManager::restartCompiler() {
    this->killCompiler();
    this->startCompiler();
}

void ExternalCompilerManager::sendMessage(const std::string& message) {
    zmq::message_t msg(message.size());
    memcpy(msg.data(), message.c_str(), message.size());
    requester.send(msg, zmq::send_flags::none);
    //Wait for the response with a timeout of 5 seconds
    zmq::pollitem_t items[] = {{static_cast<void*>(requester), 0, ZMQ_POLLIN, 0}};
    zmq::poll(items, 1, 10000);

    if (items[0].revents & ZMQ_POLLIN) {
        zmq::message_t reply;
        auto received = requester.recv(reply);
        if (received) {
            std::string replyMessage(static_cast<char*>(reply.data()), reply.size());
            QString qReplyMessage = QString::fromStdString(replyMessage);
            pMainWindow->logToolsOutput(qReplyMessage);
            if (qReplyMessage.startsWith("100")) {
                pMainWindow->logToolsOutput("COMPILED SUCCESSFULY!");
            } else {
                this->error(qReplyMessage);
            }
        } else {
            QMessageBox::warning(pMainWindow, "Error", "Failed to receive response.");
        }
    } else {
        QMessageBox::warning(pMainWindow, "Error", "No response received within the timeout period.");
    }
}

void ExternalCompilerManager::error(const QString& msg) {
    // i'd like to kill myself
    QRegularExpression regex(R"(^\[([^\]]+)\]\[(\d+),(\d+)\]\s+(.*?):\s+(.*)$)");
    QRegularExpressionMatchIterator i = regex.globalMatch(msg);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        PCompileIssue issue = std::make_shared<CompileIssue>();
        if (match.hasMatch()) {
            issue->line = match.captured(2).toInt();
            issue->column = match.captured(3).toInt();
            issue->filename = match.captured(4);
            issue->description = match.captured(5);
            issue->type = CompileIssueType::Error;
        }
        pMainWindow->onCompileIssue(issue);
    }
}

void ExternalCompilerManager::compile(const QString& filepath) {
    std::string s = "215#5#" + filepath.toStdString();
    sendMessage(s);
    sendMessage("210");
    pMainWindow->onCompileFinished(filepath,false);
}

ExternalCompilerManager::~ExternalCompilerManager() {
	compilerProcess->disconnect();
	compilerProcess->kill();
	compilerProcess->waitForFinished();
    requester.close();
    context.close();
}
