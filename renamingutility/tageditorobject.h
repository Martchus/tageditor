#ifndef RENAMINGUTILITY_SCRIPTFUNCTIONS_H
#define RENAMINGUTILITY_SCRIPTFUNCTIONS_H

#ifndef TAGEDITOR_NO_JSENGINE

#include "./jsdefs.h"

#include <QObject>

QT_FORWARD_DECLARE_CLASS(QFileInfo)

namespace RenamingUtility {

class FileSystemItem;
enum class ItemType;
enum class ActionType;

class TagEditorObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentPath READ currentPath)
    Q_PROPERTY(QString currentName READ currentName)
    Q_PROPERTY(QString currentRelativeDirectory READ currentRelativeDirectory)
    Q_PROPERTY(bool isDir READ isDir)
    Q_PROPERTY(bool isFile READ isFile)
    Q_PROPERTY(QString newName READ newName WRITE rename)
    Q_PROPERTY(QString newRelativeDirectory READ newRelativeDirectory WRITE move)

public:
    explicit TagEditorObject(TAGEDITOR_JS_ENGINE *engine);

    ActionType action() const;
    void setFileInfo(const QFileInfo &file, FileSystemItem *item);

    const QString &currentPath() const;
    const QString &currentName() const;
    const QString &currentRelativeDirectory() const;
    bool isDir() const;
    bool isFile() const;
    const QString &newName() const;
    const QString &newRelativeDirectory() const;

public slots:
    TAGEDITOR_JS_VALUE parseFileInfo(const QString &fileName);
    TAGEDITOR_JS_VALUE parseFileName(const QString &fileName);
    TAGEDITOR_JS_VALUE allFiles(const QString &dirName);
    TAGEDITOR_JS_VALUE firstFile(const QString &dirName);
    void writeLog(const QString &message);
    void rename(const QString &newName);
    void move(const QString &newRelativeDirectory);
    void skip();

private:
    TAGEDITOR_JS_ENGINE *m_engine;
    QString m_currentPath;
    QString m_currentName;
    QString m_currentRelativeDirectory;
    ItemType m_currentType;
    ActionType m_action;
    QString m_newName;
    QString m_newRelativeDirectory;
};

inline ActionType TagEditorObject::action() const
{
    return m_action;
}

} // namespace RenamingUtility

#endif // TAGEDITOR_NO_JSENGINE
#endif // RENAMINGUTILITY_SCRIPTFUNCTIONS_H
