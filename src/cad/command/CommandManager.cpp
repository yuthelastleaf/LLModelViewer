#include "CommandManager.h"
#include <QDebug>
#include <algorithm>

// ============================================
// 宏命令前置声明
// ============================================
class MacroCommand : public Command {
public:
    MacroCommand(const QString& description, std::vector<CommandPtr> commands)
        : description_(description), commands_(std::move(commands)) {}

    bool execute() override {
        // 宏命令创建时所有子命令已经执行过
        return true;
    }

    bool undo() override {
        // 逆序撤销所有子命令
        for (auto it = commands_.rbegin(); it != commands_.rend(); ++it) {
            if (!(*it)->undo()) {
                return false;
            }
        }
        return true;
    }

    bool redo() override {
        // 正序重做所有子命令
        for (auto& cmd : commands_) {
            if (!cmd->redo()) {
                return false;
            }
        }
        return true;
    }

    QString getDescription() const override {
        return description_;
    }

    QString getType() const override {
        return "MacroCommand";
    }

    size_t getMemorySize() const override {
        size_t total = sizeof(*this);
        for (const auto& cmd : commands_) {
            total += cmd->getMemorySize();
        }
        return total;
    }

private:
    QString description_;
    std::vector<CommandPtr> commands_;
};

// ============================================
// CommandManager 实现
// ============================================

CommandManager& CommandManager::instance() {
    static CommandManager instance;
    return instance;
}

bool CommandManager::executeCommand(CommandPtr command, bool canMerge) {
    if (!command) {
        qWarning() << "CommandManager: Null command";
        return false;
    }

    // 如果是批量操作模式，收集命令
    if (isRecordingMacro_) {
        if (command->execute()) {
            currentMacroCommands_.push_back(std::move(command));
            return true;
        }
        return false;
    }

    // 尝试与栈顶命令合并
    if (canMerge && tryMergeWithTop(command)) {
        emit stackChanged();
        return true;
    }

    // 执行新命令
    if (!command->execute()) {
        qWarning() << "Command execution failed:" << command->getDescription();
        return false;
    }

    // 忽略空操作
    if (command->isEmpty()) {
        return true;
    }

    // 清空重做栈
    clearRedoStack();

    // 添加到撤销栈
    undoStack_.push_back(std::move(command));

    // 清理超出限制的命令
    cleanupIfNeeded();

    emit stackChanged();
    return true;
}

bool CommandManager::undo() {
    if (!canUndo()) {
        return false;
    }

    auto command = std::move(undoStack_.back());
    undoStack_.pop_back();

    if (command->undo()) {
        redoStack_.push_back(std::move(command));
        emit stackChanged();
        return true;
    } else {
        // 撤销失败，恢复到栈中
        undoStack_.push_back(std::move(command));
        qWarning() << "Undo failed for command:" << command->getDescription();
        return false;
    }
}

bool CommandManager::redo() {
    if (!canRedo()) {
        return false;
    }

    auto command = std::move(redoStack_.back());
    redoStack_.pop_back();

    if (command->redo()) {
        undoStack_.push_back(std::move(command));
        emit stackChanged();
        return true;
    } else {
        // 重做失败，恢复到重做栈
        redoStack_.push_back(std::move(command));
        qWarning() << "Redo failed for command:" << command->getDescription();
        return false;
    }
}

void CommandManager::clear() {
    undoStack_.clear();
    redoStack_.clear();
    savePointIndex_ = 0;
    emit stackChanged();
}

bool CommandManager::canUndo() const {
    return !undoStack_.empty();
}

bool CommandManager::canRedo() const {
    return !redoStack_.empty();
}

QString CommandManager::getUndoText() const {
    if (undoStack_.empty()) {
        return "Nothing to undo";
    }
    return "Undo " + undoStack_.back()->getDescription();
}

QString CommandManager::getRedoText() const {
    if (redoStack_.empty()) {
        return "Nothing to redo";
    }
    return "Redo " + redoStack_.back()->getDescription();
}

int CommandManager::getStackDepth() const {
    return static_cast<int>(undoStack_.size());
}

size_t CommandManager::getMemoryUsage() const {
    return calculateMemoryUsage();
}

void CommandManager::beginMacro(const QString& description) {
    if (isRecordingMacro_) {
        qWarning() << "Already recording macro, ignoring beginMacro";
        return;
    }

    isRecordingMacro_ = true;
    currentMacroDescription_ = description;
    currentMacroCommands_.clear();
}

void CommandManager::endMacro() {
    if (!isRecordingMacro_) {
        qWarning() << "Not recording macro, ignoring endMacro";
        return;
    }

    isRecordingMacro_ = false;

    // 如果只有一个命令，直接添加
    if (currentMacroCommands_.size() == 1) {
        auto command = std::move(currentMacroCommands_[0]);
        currentMacroCommands_.clear();
        executeCommand(std::move(command), false);
        return;
    }

    // 如果有多个命令，创建宏命令
    if (!currentMacroCommands_.empty()) {
        auto macroCommand = std::make_unique<MacroCommand>(
            currentMacroDescription_, 
            std::move(currentMacroCommands_));
        executeCommand(std::move(macroCommand), false);
    }

    currentMacroCommands_.clear();
}

bool CommandManager::isRecordingMacro() const {
    return isRecordingMacro_;
}

void CommandManager::setMaxStackDepth(int maxDepth) {
    maxStackDepth_ = std::max(0, maxDepth);
    cleanupIfNeeded();
}

void CommandManager::setMaxMemoryUsage(size_t maxMemoryMB) {
    maxMemoryUsage_ = maxMemoryMB * 1024 * 1024; // 转换为字节
    cleanupIfNeeded();
}

void CommandManager::createSavePoint() {
    savePointIndex_ = undoStack_.size();
    
    // 清理保存点之前的旧命令以释放内存
    if (savePointIndex_ > 20) { // 保留最近20个命令
        auto keepCount = std::min(savePointIndex_, size_t(20));
        std::vector<CommandPtr> newStack;
        newStack.reserve(keepCount);
        
        auto startIt = undoStack_.end() - keepCount;
        for (auto it = startIt; it != undoStack_.end(); ++it) {
            newStack.push_back(std::move(*it));
        }
        
        undoStack_ = std::move(newStack);
        savePointIndex_ = 0; // 重置保存点
    }
    
    emit stackChanged();
}

bool CommandManager::hasUnsavedChanges() const {
    return undoStack_.size() != savePointIndex_;
}

// ============================================
// 私有辅助方法
// ============================================

bool CommandManager::tryMergeWithTop(CommandPtr& command) {
    if (undoStack_.empty()) {
        return false;
    }

    auto& topCommand = undoStack_.back();
    if (topCommand->canMergeWith(command.get())) {
        return topCommand->mergeWith(std::move(command));
    }

    return false;
}

void CommandManager::cleanupIfNeeded() {
    // 检查栈深度限制
    if (maxStackDepth_ > 0) {
        while (undoStack_.size() > static_cast<size_t>(maxStackDepth_)) {
            undoStack_.erase(undoStack_.begin());
            if (savePointIndex_ > 0) {
                savePointIndex_--;
            }
        }
    }

    // 检查内存限制
    if (maxMemoryUsage_ > 0) {
        size_t currentUsage = calculateMemoryUsage();
        if (currentUsage > maxMemoryUsage_) {
            emit memoryWarning(currentUsage / (1024 * 1024), maxMemoryUsage_ / (1024 * 1024));
            
            // 移除最旧的命令直到内存在限制内
            while (!undoStack_.empty() && calculateMemoryUsage() > maxMemoryUsage_) {
                undoStack_.erase(undoStack_.begin());
                if (savePointIndex_ > 0) {
                    savePointIndex_--;
                }
            }
        }
    }
}

size_t CommandManager::calculateMemoryUsage() const {
    size_t total = 0;
    for (const auto& cmd : undoStack_) {
        total += cmd->getMemorySize();
    }
    for (const auto& cmd : redoStack_) {
        total += cmd->getMemorySize();
    }
    return total;
}

void CommandManager::clearRedoStack() {
    redoStack_.clear();
}