#include "EntityCommands.h"
#include "../transform/Transform.h"
#include <QDebug>

// ============================================
// MoveCommand 实现
// ============================================

MoveCommand::MoveCommand(Document* document, 
                        const std::vector<EntityId>& entityIds, 
                        const glm::vec3& offset)
    : document_(document)
    , entityIds_(entityIds)
    , totalOffset_(offset)
    , currentOffset_(offset) {
}

bool MoveCommand::execute() {
    if (!validateEntities()) {
        return false;
    }

    performMove(currentOffset_);
    executed_ = true;
    return true;
}

bool MoveCommand::undo() {
    if (!executed_ || !validateEntities()) {
        return false;
    }

    performMove(-totalOffset_);
    return true;
}

bool MoveCommand::redo() {
    if (!validateEntities()) {
        return false;
    }

    performMove(totalOffset_);
    return true;
}

QString MoveCommand::getDescription() const {
    if (entityIds_.size() == 1) {
        return QString("Move Entity %1").arg(entityIds_[0]);
    } else {
        return QString("Move %1 Entities").arg(entityIds_.size());
    }
}

QString MoveCommand::getType() const {
    return "MoveCommand";
}

bool MoveCommand::canMergeWith(const Command* other) const {
    auto moveCmd = dynamic_cast<const MoveCommand*>(other);
    if (!moveCmd) {
        return false;
    }

    // 只有移动相同实体集合才能合并
    if (entityIds_.size() != moveCmd->entityIds_.size()) {
        return false;
    }

    for (size_t i = 0; i < entityIds_.size(); ++i) {
        if (entityIds_[i] != moveCmd->entityIds_[i]) {
            return false;
        }
    }

    return true;
}

bool MoveCommand::mergeWith(std::unique_ptr<Command> other) {
    auto moveCmd = dynamic_cast<MoveCommand*>(other.get());
    if (!moveCmd) {
        return false;
    }

    // 累积偏移量
    totalOffset_ += moveCmd->currentOffset_;
    return true;
}

bool MoveCommand::isEmpty() const {
    const float epsilon = 1e-6f;
    return glm::length(totalOffset_) < epsilon;
}

size_t MoveCommand::getMemorySize() const {
    return sizeof(*this) + entityIds_.size() * sizeof(EntityId);
}

void MoveCommand::performMove(const glm::vec3& offset) {
    std::vector<Entity*> entities;
    for (EntityId id : entityIds_) {
        if (auto entity = document_->get(id)) {
            entities.push_back(entity);
        }
    }

    if (!entities.empty()) {
        Transform::translateEntities(entities, offset);
        
        // 通知文档更新
        for (EntityId id : entityIds_) {
            document_->notifyEntityChanged(id);
        }
    }
}

bool MoveCommand::validateEntities() const {
    for (EntityId id : entityIds_) {
        if (!document_->get(id)) {
            qWarning() << "MoveCommand: Entity" << id << "no longer exists";
            return false;
        }
    }
    return !entityIds_.empty();
}

// ============================================
// AddEntityCommand 实现
// ============================================

AddEntityCommand::AddEntityCommand(Document* document, EntityPtr entity)
    : document_(document), entity_(std::move(entity)) {
}

bool AddEntityCommand::execute() {
    if (!entity_) {
        return false;
    }

    // 解引用智能指针获取Entity对象
    entityId_ = document_->add(*entity_);
    executed_ = true;
    return entityId_ != EntityId(-1);
}

bool AddEntityCommand::undo() {
    if (!executed_) {
        return false;
    }

    entity_ = document_->removeAndTake(entityId_);
    return entity_ != nullptr;
}

QString AddEntityCommand::getDescription() const {
    if (entity_) {
        return QString("Add %1").arg(static_cast<int>(entity_->type));
    }
    return "Add Entity";
}

QString AddEntityCommand::getType() const {
    return "AddEntityCommand";
}

size_t AddEntityCommand::getMemorySize() const {
    size_t size = sizeof(*this);
    if (entity_) {
        size += sizeof(*entity_);
        // 添加几何数据的估算大小
        size += 64; // 估算值，实际应根据几何类型计算
    }
    return size;
}

// ============================================
// DeleteEntityCommand 实现
// ============================================

DeleteEntityCommand::DeleteEntityCommand(Document* document, const std::vector<EntityId>& entityIds)
    : document_(document), entityIds_(entityIds) {
}

bool DeleteEntityCommand::execute() {
    deletedEntities_.clear();
    deletedEntities_.reserve(entityIds_.size());

    for (EntityId id : entityIds_) {
        if (auto entity = document_->removeAndTake(id)) {
            deletedEntities_.push_back(std::move(entity));
        } else {
            // 恢复已删除的实体
            for (size_t i = 0; i < deletedEntities_.size(); ++i) {
                document_->addWithId(entityIds_[i], std::move(deletedEntities_[i]));
            }
            deletedEntities_.clear();
            return false;
        }
    }

    executed_ = true;
    return true;
}

bool DeleteEntityCommand::undo() {
    if (!executed_) {
        return false;
    }

    for (size_t i = 0; i < entityIds_.size() && i < deletedEntities_.size(); ++i) {
        document_->addWithId(entityIds_[i], std::move(deletedEntities_[i]));
    }

    deletedEntities_.clear();
    return true;
}

QString DeleteEntityCommand::getDescription() const {
    if (entityIds_.size() == 1) {
        return QString("Delete Entity %1").arg(entityIds_[0]);
    } else {
        return QString("Delete %1 Entities").arg(entityIds_.size());
    }
}

QString DeleteEntityCommand::getType() const {
    return "DeleteEntityCommand";
}

size_t DeleteEntityCommand::getMemorySize() const {
    size_t size = sizeof(*this) + entityIds_.size() * sizeof(EntityId);
    for (const auto& entity : deletedEntities_) {
        if (entity) {
            size += sizeof(*entity) + 64; // 几何数据估算
        }
    }
    return size;
}

// ============================================
// ModifyEntityCommand 实现
// ============================================

ModifyEntityCommand::ModifyEntityCommand(Document* document, 
                                       EntityId entityId,
                                       const Style& newStyle,
                                       const Style& oldStyle)
    : document_(document)
    , entityId_(entityId)
    , newStyle_(newStyle)
    , oldStyle_(oldStyle) {
}

bool ModifyEntityCommand::execute() {
    if (auto entity = document_->get(entityId_)) {
        entity->style = newStyle_;
        document_->notifyEntityChanged(entityId_);
        executed_ = true;
        return true;
    }
    return false;
}

bool ModifyEntityCommand::undo() {
    if (!executed_) {
        return false;
    }

    if (auto entity = document_->get(entityId_)) {
        entity->style = oldStyle_;
        document_->notifyEntityChanged(entityId_);
        return true;
    }
    return false;
}

QString ModifyEntityCommand::getDescription() const {
    return QString("Modify Entity %1").arg(entityId_);
}

QString ModifyEntityCommand::getType() const {
    return "ModifyEntityCommand";
}

bool ModifyEntityCommand::canMergeWith(const Command* other) const {
    auto modifyCmd = dynamic_cast<const ModifyEntityCommand*>(other);
    return modifyCmd && modifyCmd->entityId_ == entityId_;
}

bool ModifyEntityCommand::mergeWith(std::unique_ptr<Command> other) {
    auto modifyCmd = dynamic_cast<ModifyEntityCommand*>(other.get());
    if (!modifyCmd || modifyCmd->entityId_ != entityId_) {
        return false;
    }

    // 保持原始的 oldStyle_，更新最终的 newStyle_
    newStyle_ = modifyCmd->newStyle_;
    return true;
}

size_t ModifyEntityCommand::getMemorySize() const {
    return sizeof(*this);
}