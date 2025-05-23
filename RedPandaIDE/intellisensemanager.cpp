#include "intellisensemanager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QCoreApplication>
#include <QJsonArray>
#include <zmq.hpp>
#include "mainwindow.h"
#include "qsynedit/qsynedit.h"
#include "editor.h"
#include "editorlist.h"
#include "editor.h"


IntelliSenseManager::IntelliSenseManager(QObject *parent)
    : QObject{parent}, requestId(0), context(1),
    requester(context, zmq::socket_type::req)
{
    compilerProcess = new QProcess(this);
    requester.connect("tcp://127.0.0.1:5557");
    requester.set(zmq::sockopt::reconnect_ivl, 100); // 100ms between retries
    requester.set(zmq::sockopt::reconnect_ivl_max, 5000); // Max 5s delay
}

IntelliSenseManager& IntelliSenseManager::instance()
{
    static IntelliSenseManager instance(nullptr);
    return instance;
}

void IntelliSenseManager::connectEvents() {
    initializeLSP("");
}

void IntelliSenseManager::initializeLSP(const QString& filename) {
    QJsonObject lspParams;

    // Process ID of the current application
    lspParams["processId"] = static_cast<int>(QCoreApplication::applicationPid());

    // Provide a valid rootUri based on the file's directory
    QUrl rootUrl = QUrl::fromLocalFile(QFileInfo(filename).canonicalFilePath());
    lspParams["rootUri"] = rootUrl.toString();  // e.g., file:///path/to/project

    // Define basic client capabilities
    QJsonObject synchronization;
    synchronization["didSave"] = true;
    synchronization["willSave"] = true;
    synchronization["willSaveWaitUntil"] = false;

    QJsonObject completionItem;
    completionItem["snippetSupport"] = true;

    QJsonObject completion;
    completion["completionItem"] = completionItem;

    QJsonObject textDocument;
    textDocument["synchronization"] = synchronization;
    textDocument["completion"] = completion;

    QJsonObject workspace;
    workspace["applyEdit"] = true;

    QJsonObject capabilities;
    capabilities["workspace"] = workspace;
    capabilities["textDocument"] = textDocument;

    lspParams["capabilities"] = capabilities;

    // Final JSON-RPC message
    QJsonObject resultJSON;
    resultJSON["jsonrpc"] = "2.0";
    resultJSON["id"] = requestId++;
    resultJSON["method"] = "initialize";
    resultJSON["params"] = lspParams;

    QString data = QJsonDocument(resultJSON).toJson(QJsonDocument::Compact);
    sendMessage(data.toStdString());


}


QStringList IntelliSenseManager::callIntelli(const QSynedit::BufferCoord& pos, const QString& filename, const QString request_type) {
    QJsonObject lspPosition;
    lspPosition["line"] = pos.line - 1;
    lspPosition["character"] = pos.ch - 1;

    QJsonObject textDocument;

    QUrl rootUrl = QUrl::fromLocalFile(QFileInfo(filename).canonicalFilePath());
    textDocument["uri"] = rootUrl.toString();

    QJsonObject lspParams;
    lspParams["textDocument"] = textDocument;
    lspParams["position"] = lspPosition;

    QJsonObject resultJSON;
    resultJSON["jsonrpc"] = "2.0";
    resultJSON["id"] = requestId++;
    resultJSON["method"] = "textDocument/"+ request_type;
    resultJSON["params"] = lspParams;

    QString data = QJsonDocument(resultJSON).toJson(QJsonDocument::Compact);
    QString result = sendMessage(data.toStdString());
    if (request_type == "hover") {
        return QStringList() << QJsonDocument::fromJson(result.toUtf8()).object()["result"].toObject()["contents"].toObject()["value"].toString();
    } else if (request_type == "completion") {
        QStringList res;
        QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());

        QJsonArray resultArray = doc.object()["result"].toArray();
        for (const QJsonValue& item : resultArray) {
            if (item.isObject()) {
                QJsonObject itemObj = item.toObject();
                if (itemObj.contains("label") && itemObj["label"].isString()) {
                    res << itemObj["label"].toString();
                }
            }
        }
        return res;
    }
}

void IntelliSenseManager::didChange(const QString& filename, const QString& fulltext) {
    if (!documentVersions.contains(filename)) {
        documentVersions[filename] = 0;
    }

    // Build the JSON-RPC message
    QJsonObject message {
        {"jsonrpc", "2.0"},
        {"method", "textDocument/didChange"},
        {"params", QJsonObject {
                       {"textDocument", QJsonObject {
                                            {"uri", QUrl::fromLocalFile(QFileInfo(filename).canonicalFilePath()).toString()},
                                            {"version", ++documentVersions[filename]}
                                        }},
                       {"contentChanges", QJsonArray {
                                              QJsonObject {
                                                  {"text", fulltext}
                                              }
                                          }}
                   }}
    };

    // Convert to JSON
    QJsonDocument doc(message);
    QByteArray data = doc.toJson(QJsonDocument::Compact);

    qDebug() << data;

    sendMessage(data.toStdString());
}

void IntelliSenseManager::startIntelli() {
    compilerProcess->setProgram("D:\\Sci\\TestIntelli\\bin\\Debug\\TestIntelli.exe");
    /*compilerProcess->setArguments(QStringList() << "/noconsole" << "commandmode");*/

    // Connect signals for output and errors
    //connect(compilerProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::handlePascalOutput);

    compilerProcess->start();
    if (!compilerProcess->waitForStarted()) {
        qDebug() << "Failed to start process!";
    } else {
        qDebug() << "IntelliSense Process started successfully.";
    }
}

void IntelliSenseManager::didOpen(const QString& filename, Editor* editor) {
    QJsonObject textDocument;

    QUrl fileUrl = QUrl::fromLocalFile(QFileInfo(filename).canonicalFilePath());
    textDocument["uri"] = fileUrl.toString(QUrl::FullyEncoded);
    textDocument["languageId"] = "pas"; // Adjust based on file type
    textDocument["version"] = 0;
    textDocument["text"] = editor->text(); // Ensure this is the full file content

    QJsonObject lspParams;
    lspParams["textDocument"] = textDocument;

    QJsonObject resultJSON;
    resultJSON["jsonrpc"] = "2.0";
    resultJSON["method"] = "textDocument/didOpen"; // No "id" needed for notifications
    resultJSON["params"] = lspParams;

    sendMessage(QJsonDocument(resultJSON).toJson(QJsonDocument::Compact).toStdString());


    connect(editor, &QSynedit::QSynEdit::changeForIntelli, this, [this, filename](const QString& text) {
         this->IntelliSenseManager::didChange(filename, text);
    });
}

QString IntelliSenseManager::sendMessage(const std::string& message) {
    try {
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
                qDebug() << qReplyMessage;
                // pMainWindow->logToolsOutput(qReplyMessage);
                // pMainWindow->logToolsOutput("IntelliSense has answered!");
                return qReplyMessage;
            } else {
                // pMainWindow->logToolsOutput("Unexpected error while receiving IntelliSense answer");
            }
        } else {
            // pMainWindow->logToolsOutput("IntelliSense is not responding");
        }
    }
    catch (...) {
        // pMainWindow->logToolsOutput("Unknown error occured");
    }
    return "";
}
