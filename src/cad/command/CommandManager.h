#pragma once

#include "Command.h"
#include <QObject>
#include <QKeySequence>
#include <QUndoStack>
#include <vector>
#include <memory>
#include <functional>

/**
 * ✅ 命令栈管理器：全局单例，管理所有可撤销操作
 * 
 * 功能特性：
 * 1. 命令执行、撤销、重做
 * 2. 智能命令合并（连续相似操作）
 * 3. 内存控制（限制栈深度和内存使用）
 * 4. 保存点管理（文件保存时清理）
 * 5. 批量操作支持（宏命令）
 */
class CommandManager : public QObject {
    Q_OBJECT

public:
    static CommandManager& instance();

    // ============================================
    // 基本命令操作
    // ============================================

    /**
     * 执行命令并添加到栈中
     * @param command 要执行的命令
     * @param canMerge 是否允许与前一个命令合并
     * @return true 成功执行
     */
    bool executeCommand(CommandPtr command, bool canMerge = true);

    /**
     * 撤销操作 (Ctrl+Z)
     */
    bool undo();

    /**
     * 重做操作 (Ctrl+Y 或 Ctrl+Shift+Z)
     */
    bool redo();

    /**
     * 清空整个命令栈
     */
    void clear();

    // ============================================
    // 查询状态
    // ============================================

    /**
     * 是否可以撤销
     */
    bool canUndo() const;

    /**
     * 是否可以重做
     */
    bool canRedo() const;

    /**
     * 获取下一个撤销命令的描述
     */
    QString getUndoText() const;

    /**
     * 获取下一个重做命令的描述
     */
    QString getRedoText() const;

    /**
     * 获取当前栈深度
     */
    int getStackDepth() const;

    /**
     * 获取内存使用量（字节）
     */
    size_t getMemoryUsage() const;

    // ============================================
    // 批量操作（宏命令）
    // ============================================

    /**
     * 开始批量操作记录
     * @param description 批量操作的描述
     */
    void beginMacro(const QString& description);

    /**
     * 结束批量操作记录
     */
    void endMacro();

    /**
     * 是否正在记录批量操作
     */
    bool isRecordingMacro() const;

    // ============================================
    // 内存和性能管理
    // ============================================

    /**
     * 设置最大栈深度
     * @param maxDepth 最大深度，0表示无限制
     */
    void setMaxStackDepth(int maxDepth);

    /**
     * 设置最大内存使用量
     * @param maxMemoryMB 最大内存MB，0表示无限制
     */
    void setMaxMemoryUsage(size_t maxMemoryMB);

    /**
     * 创建保存点（文件保存时调用）
     * 清空旧命令，从此点开始记录新操作
     */
    void createSavePoint();

    /**
     * 是否有未保存的修改
     */
    bool hasUnsavedChanges() const;

signals:
    /**
     * 栈状态变化信号（用于更新UI）
     */
    void stackChanged();

    /**
     * 内存使用过高警告
     */
    void memoryWarning(size_t currentMB, size_t limitMB);

private:
    CommandManager() = default;
    ~CommandManager() = default;
    CommandManager(const CommandManager&) = delete;
    CommandManager& operator=(const CommandManager&) = delete;

    // ============================================
    // 内部数据结构
    // ============================================

    std::vector<CommandPtr> undoStack_;     // 撤销栈
    std::vector<CommandPtr> redoStack_;     // 重做栈
    
    // 批量操作支持
    bool isRecordingMacro_ = false;
    QString currentMacroDescription_;
    std::vector<CommandPtr> currentMacroCommands_;

    // 性能配置
    int maxStackDepth_ = 50;                // 默认最大50层撤销
    size_t maxMemoryUsage_ = 100 * 1024 * 1024;  // 默认最大100MB
    
    // 保存点管理
    size_t savePointIndex_ = 0;             // 保存点位置
    
    // ============================================
    // 内部辅助方法
    // ============================================
    
    /**
     * 尝试与栈顶命令合并
     */
    bool tryMergeWithTop(CommandPtr& command);

    /**
     * 清理超出限制的旧命令
     */
    void cleanupIfNeeded();

    /**
     * 计算当前内存使用
     */
    size_t calculateMemoryUsage() const;

    /**
     * 清空重做栈（新命令执行时）
     */
    void clearRedoStack();
};