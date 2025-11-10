#pragma once

#include "Command.h"
#include "../data/Document.h"
#include <glm/glm.hpp>
#include <vector>

/**
 * ✅ 移动实体命令
 * 
 * 功能：
 * 1. 移动单个或多个实体
 * 2. 支持命令合并（连续移动）
 * 3. 完整的撤销/重做支持
 */
class MoveCommand : public Command {
public:
    /**
     * 构造移动命令
     * @param document 文档对象
     * @param entityIds 要移动的实体ID列表
     * @param offset 移动偏移量
     */
    MoveCommand(Document* document, 
                const std::vector<EntityId>& entityIds, 
                const glm::vec3& offset);

    // Command 接口实现
    bool execute() override;
    bool undo() override;
    bool redo() override;
    
    QString getDescription() const override;
    QString getType() const override;
    
    bool canMergeWith(const Command* other) const override;
    bool mergeWith(std::unique_ptr<Command> other) override;
    bool isEmpty() const override;
    
    size_t getMemorySize() const override;

private:
    Document* document_;
    std::vector<EntityId> entityIds_;
    glm::vec3 totalOffset_;       // 累积的总偏移量
    glm::vec3 currentOffset_;     // 当前这次的偏移量
    
    // 执行状态
    bool executed_ = false;
    
    /**
     * 实际执行移动操作
     */
    void performMove(const glm::vec3& offset);
    
    /**
     * 检查实体是否仍然存在
     */
    bool validateEntities() const;
};

/**
 * ✅ 添加实体命令
 */
class AddEntityCommand : public Command {
public:
    AddEntityCommand(Document* document, EntityPtr entity);
    
    bool execute() override;
    bool undo() override;
    QString getDescription() const override;
    QString getType() const override;
    size_t getMemorySize() const override;

private:
    Document* document_;
    EntityPtr entity_;
    EntityId entityId_;
    bool executed_ = false;
};

/**
 * ✅ 删除实体命令
 */
class DeleteEntityCommand : public Command {
public:
    DeleteEntityCommand(Document* document, const std::vector<EntityId>& entityIds);
    
    bool execute() override;
    bool undo() override;
    QString getDescription() const override;
    QString getType() const override;
    size_t getMemorySize() const override;

private:
    Document* document_;
    std::vector<EntityId> entityIds_;
    std::vector<EntityPtr> deletedEntities_; // 保存被删除的实体
    bool executed_ = false;
};

/**
 * ✅ 修改实体属性命令
 */
class ModifyEntityCommand : public Command {
public:
    ModifyEntityCommand(Document* document, 
                       EntityId entityId,
                       const Style& newStyle,
                       const Style& oldStyle);
    
    bool execute() override;
    bool undo() override;
    QString getDescription() const override;
    QString getType() const override;
    
    bool canMergeWith(const Command* other) const override;
    bool mergeWith(std::unique_ptr<Command> other) override;
    
    size_t getMemorySize() const override;

private:
    Document* document_;
    EntityId entityId_;
    Style newStyle_;
    Style oldStyle_;
    bool executed_ = false;
};