#pragma once

#include "Command.h"
#include <QObject>
#include <vector>
#include <memory>

/**
 * 命令栈管理器：全局单例，管理Undo/Redo操作
 * 特性：智能合并、内存控制、保存点、批量操作
 */
class CommandManager : public QObject {
    Q_OBJECT

public:
    static CommandManager& instance();

    // 基本操作
    bool executeCommand(CommandPtr command, bool canMerge = true);
    bool undo();
    bool redo();
    void clear();

    // 状态查询
    bool canUndo() const;
    bool canRedo() const;
    QString getUndoText() const;
    QString getRedoText() const;
    int getStackDepth() const;
    size_t getMemoryUsage() const;

    // 批量操作（宏命令）
    void beginMacro(const QString& description);
    void endMacro();
    bool isRecordingMacro() const;

    // 配置
    void setMaxStackDepth(int maxDepth);
    void setMaxMemoryUsage(size_t maxMemoryMB);
    void createSavePoint();
    bool hasUnsavedChanges() const;

signals:
    void stackChanged();
    void memoryWarning(size_t currentMB, size_t limitMB);

private:
    CommandManager() = default;
    ~CommandManager() = default;
    CommandManager(const CommandManager&) = delete;
    CommandManager& operator=(const CommandManager&) = delete;

    std::vector<CommandPtr> undoStack_;
    std::vector<CommandPtr> redoStack_;
    
    bool isRecordingMacro_ = false;
    QString currentMacroDescription_;
    std::vector<CommandPtr> currentMacroCommands_;

    int maxStackDepth_ = 50;
    size_t maxMemoryUsage_ = 100 * 1024 * 1024;  // 100MB
    size_t savePointIndex_ = 0;
    
    bool tryMergeWithTop(CommandPtr& command);
    void cleanupIfNeeded();
    size_t calculateMemoryUsage() const;
    void clearRedoStack();
};
