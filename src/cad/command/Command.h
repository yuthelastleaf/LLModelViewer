#pragma once

#include <memory>
#include <QString>

/**
 * 命令接口：所有可撤销操作的基类
 */
class Command {
public:
    virtual ~Command() = default;

    virtual bool execute() = 0;
    virtual bool undo() = 0;
    virtual bool redo() { return execute(); }

    virtual QString getDescription() const = 0;
    virtual QString getType() const = 0;

    // 命令合并支持
    virtual bool canMergeWith(const Command* other) const { Q_UNUSED(other); return false; }
    virtual bool mergeWith(std::unique_ptr<Command> other) { Q_UNUSED(other); return false; }
    
    virtual bool isEmpty() const { return false; }
    virtual size_t getMemorySize() const { return sizeof(*this); }
};

using CommandPtr = std::unique_ptr<Command>;
