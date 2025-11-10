#pragma once

#include <memory>
#include <string>
#include <QObject>

/**
 * ✅ 命令接口：所有可撤销操作的基类
 * 
 * 设计原则：
 * 1. 每个命令都知道如何执行和撤销自己
 * 2. 命令保存执行前后的状态信息
 * 3. 支持合并相似命令（如连续移动）
 */
class Command {
public:
    virtual ~Command() = default;

    /**
     * 执行命令
     * @return true 成功，false 失败
     */
    virtual bool execute() = 0;

    /**
     * 撤销命令
     * @return true 成功，false 失败
     */
    virtual bool undo() = 0;

    /**
     * 重做命令（默认实现为重新执行）
     * @return true 成功，false 失败
     */
    virtual bool redo() { return execute(); }

    /**
     * 获取命令描述（用于UI显示）
     */
    virtual QString getDescription() const = 0;

    /**
     * 获取命令类型（用于合并判断）
     */
    virtual QString getType() const = 0;

    /**
     * 判断是否可以与其他命令合并
     * @param other 另一个命令
     * @return true 可以合并
     */
    virtual bool canMergeWith(const Command* other) const { 
        Q_UNUSED(other); 
        return false; 
    }

    /**
     * 与另一个命令合并
     * @param other 要合并的命令
     * @return true 成功合并
     */
    virtual bool mergeWith(std::unique_ptr<Command> other) { 
        Q_UNUSED(other); 
        return false; 
    }

    /**
     * 判断命令是否为空操作（可以被忽略）
     */
    virtual bool isEmpty() const { return false; }

    /**
     * 获取命令占用的内存大小（用于内存管理）
     */
    virtual size_t getMemorySize() const { return sizeof(*this); }
};

using CommandPtr = std::unique_ptr<Command>;