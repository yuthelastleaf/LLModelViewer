#include "SelectionManager.h"
#include <QDebug>
#include <algorithm>

SelectionManager::SelectionManager(Document* document, QObject* parent)
    : QObject(parent)
    , document_(document)
{
}

// ============================================
// 查询选择状态
// ============================================

std::vector<Entity*> SelectionManager::getSelectedEntities() const {
    std::vector<Entity*> entities;
    entities.reserve(selectedIds_.size());
    
    for (EntityId id : selectedIds_) {
        if (Entity* entity = document_->get(id)) {
            entities.push_back(entity);
        }
    }
    
    return entities;
}

// ============================================
// 修改选择状态
// ============================================

void SelectionManager::clearSelection() {
    if (selectedIds_.empty()) {
        return;  // 已经是空的，不需要通知
    }
    
    selectedIds_.clear();
    syncToDocument();
    notifySelectionChanged();
    
    qDebug() << "Selection cleared";
}

void SelectionManager::select(EntityId id) {
    // 检查实体是否存在
    if (!document_->get(id)) {
        qWarning() << "Cannot select non-existent entity:" << id;
        return;
    }
    
    // 如果已经只选中这一个，不需要更新
    if (selectedIds_.size() == 1 && selectedIds_.count(id) > 0) {
        return;
    }
    
    selectedIds_.clear();
    selectedIds_.insert(id);
    syncToDocument();
    notifySelectionChanged();
    
    qDebug() << "Selected entity:" << id;
}

void SelectionManager::select(const std::vector<EntityId>& ids) {
    selectedIds_.clear();
    
    for (EntityId id : ids) {
        if (document_->get(id)) {
            selectedIds_.insert(id);
        }
    }
    
    syncToDocument();
    notifySelectionChanged();
    
    qDebug() << "Selected" << selectedIds_.size() << "entities";
}

void SelectionManager::addToSelection(EntityId id) {
    if (!document_->get(id)) {
        qWarning() << "Cannot add non-existent entity to selection:" << id;
        return;
    }
    
    if (selectedIds_.count(id) > 0) {
        return;  // 已经选中
    }
    
    selectedIds_.insert(id);
    syncToDocument();
    notifySelectionChanged();
    
    qDebug() << "Added to selection:" << id;
}

void SelectionManager::addToSelection(const std::vector<EntityId>& ids) {
    bool changed = false;
    
    for (EntityId id : ids) {
        if (document_->get(id) && selectedIds_.count(id) == 0) {
            selectedIds_.insert(id);
            changed = true;
        }
    }
    
    if (changed) {
        syncToDocument();
        notifySelectionChanged();
        qDebug() << "Added" << ids.size() << "entities to selection";
    }
}

void SelectionManager::removeFromSelection(EntityId id) {
    if (selectedIds_.erase(id) > 0) {
        syncToDocument();
        notifySelectionChanged();
        qDebug() << "Removed from selection:" << id;
    }
}

void SelectionManager::removeFromSelection(const std::vector<EntityId>& ids) {
    bool changed = false;
    
    for (EntityId id : ids) {
        if (selectedIds_.erase(id) > 0) {
            changed = true;
        }
    }
    
    if (changed) {
        syncToDocument();
        notifySelectionChanged();
        qDebug() << "Removed" << ids.size() << "entities from selection";
    }
}

void SelectionManager::toggleSelection(EntityId id) {
    if (!document_->get(id)) {
        return;
    }
    
    if (selectedIds_.count(id) > 0) {
        selectedIds_.erase(id);
    } else {
        selectedIds_.insert(id);
    }
    
    syncToDocument();
    notifySelectionChanged();
    
    qDebug() << "Toggled selection:" << id;
}

void SelectionManager::selectAll() {
    selectedIds_.clear();
    
    for (const auto* entity : document_->all()) {
        if (entity) {
            selectedIds_.insert(entity->id);
        }
    }
    
    syncToDocument();
    notifySelectionChanged();
    
    qDebug() << "Selected all:" << selectedIds_.size() << "entities";
}

void SelectionManager::invertSelection() {
    std::unordered_set<EntityId> newSelection;
    
    for (const auto* entity : document_->all()) {
        if (entity && selectedIds_.count(entity->id) == 0) {
            newSelection.insert(entity->id);
        }
    }
    
    selectedIds_ = std::move(newSelection);
    syncToDocument();
    notifySelectionChanged();
    
    qDebug() << "Inverted selection:" << selectedIds_.size() << "entities";
}

// ============================================
// 选择模式
// ============================================

void SelectionManager::selectWithMode(EntityId id, SelectMode mode) {
    switch (mode) {
        case SelectMode::REPLACE:
            select(id);
            break;
        case SelectMode::ADD:
            addToSelection(id);
            break;
        case SelectMode::TOGGLE:
            toggleSelection(id);
            break;
    }
}

void SelectionManager::selectWithMode(const std::vector<EntityId>& ids, SelectMode mode) {
    switch (mode) {
        case SelectMode::REPLACE:
            select(ids);
            break;
        case SelectMode::ADD:
            addToSelection(ids);
            break;
        case SelectMode::TOGGLE:
            for (EntityId id : ids) {
                toggleSelection(id);
            }
            break;
    }
}

// ============================================
// 内部方法
// ============================================

void SelectionManager::syncToDocument() {
    // 清除所有实体的选中状态
    for (auto* entity : document_->all()) {
        if (entity) {
            entity->selected = false;
        }
    }
    
    // 设置选中实体的状态
    for (EntityId id : selectedIds_) {
        if (Entity* entity = document_->get(id)) {
            entity->selected = true;
        }
    }
}

void SelectionManager::notifySelectionChanged() {
    emit selectionChanged(static_cast<int>(selectedIds_.size()));
    
    std::vector<EntityId> ids(selectedIds_.begin(), selectedIds_.end());
    emit selectedEntitiesChanged(ids);
}