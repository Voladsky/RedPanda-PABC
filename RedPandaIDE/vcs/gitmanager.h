#ifndef GITMANAGER_H
#define GITMANAGER_H

#include <QObject>
#include <QFileInfo>
#include <QSet>
#include "utils.h"
#include "gitutils.h"

class GitError: public BaseError {
public:
    explicit GitError(const QString& reason);
};

class GitManager : public QObject
{
    Q_OBJECT
public:

    explicit GitManager(QObject *parent = nullptr);

    void createRepository(const QString& folder);
    bool hasRepository(const QString& folder, QString& currentBranch);

    QString rootFolder(const QString& folder);

    bool isFileInRepository(const QFileInfo& fileInfo);
    bool isFileStaged(const QFileInfo& fileInfo);
    bool isFileChanged(const QFileInfo& fileInfo);

    void add(const QString& folder, const QString& path);
    void remove(const QString& folder, const QString& path);
    void rename(const QString& folder, const QString& oldName, const QString& newName);
    void restore(const QString& folder, const QString& path);

    QStringList listFiles(const QString& folder);
    QStringList listStagedFiles(const QString& folder);
    QStringList listChangedFiles(const QString& folder);
    QStringList listBranches(const QString& folder, int& current);
    bool switchToBranch(const QString& folder, const QString& branch, bool create,
                        bool force, bool merge, bool track, bool noTrack, bool forceCreation );

    void clone(const QString& folder, const QString& url);
    void commit(const QString& folder, const QString& message, bool autoStage);
    void revert(const QString& folder);
    void reset(const QString& folder, const QString& commit, GitResetStrategy strategy);

    bool isValid();

signals:
    void gitCmdRunning(const QString& gitCmd);
    void gitCmdFinished(const QString& message);
private:
    QString runGit(const QString& workingFolder, const QStringList& args);

    QString escapeUTF8String(const QByteArray& rawString);
private:
};

#endif // GITMANAGER_H
