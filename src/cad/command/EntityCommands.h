#pragma once

#include "Command.h"
#include "../data/Document.h"
#include <glm/glm.hpp>
#include <vector>
#include <chrono>

// 移动实体命令（支持智能合并）
class MoveCommand : public Command {
public:
    MoveCommand(Document* document, const std::vector<EntityId>& entityIds, const glm::vec3& offset);

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
    glm::vec3 totalOffset_;
    glm::vec3 currentOffset_;
    bool executed_ = false;
    
    std::chrono::steady_clock::time_point timestamp_;
    static constexpr int MERGE_TIME_THRESHOLD_MS = 500;
    
    void performMove(const glm::vec3& offset);
    bool validateEntities() const;
};

// 添加实体命令
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

// 删除实体命令
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
    std::vector<EntityPtr> deletedEntities_;
    bool executed_ = false;
};

// 修改实体属性命令
class ModifyEntityCommand : public Command {
public:
    ModifyEntityCommand(Document* document, EntityId entityId, const Style& newStyle, const Style& oldStyle);
    
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
