#pragma once
#include "../data/document.h"
#include <unordered_set>
#include <vector>
#include <QObject>

/**
 * SelectionManager - 选择管理器
 * 
 * 功能：
 * - 管理当前选择集合
 * - 支持单选、多选、全选、反选
 * - 发出选择变化信号
 */
class SelectionManager : public QObject {
    Q_OBJECT

public:
    explicit SelectionManager(Document* document, QObject* parent = nullptr);

    // ============================================
    // 查询选择状态
    // ============================================
    
    /**
     * 获取所有选中的实体 ID
     */
    const std::unordered_set<EntityId>& getSelectedIds() const { 
        return selectedIds_; 
    }
    
    /**
     * 获取选中的实体列表
     */
    std::vector<Entity*> getSelectedEntities() const;
    
    /**
     * 判断实体是否被选中
     */
    bool isSelected(EntityId id) const {
        return selectedIds_.count(id) > 0;
    }
    
    /**
     * 获取选中数量
     */
    size_t getSelectionCount() const {
        return selectedIds_.size();
    }
    
    /**
     * 是否有选中
     */
    bool hasSelection() const {
        return !selectedIds_.empty();
    }

    // ============================================
    // 修改选择状态
    // ============================================
    
    /**
     * 清空选择
     */
    void clearSelection();
    
    /**
     * 选中单个实体（替换当前选择）
     */
    void select(EntityId id);
    
    /**
     * 选中多个实体（替换当前选择）
     */
    void select(const std::vector<EntityId>& ids);
    
    /**
     * 添加到选择（保留原有选择）
     */
    void addToSelection(EntityId id);
    void addToSelection(const std::vector<EntityId>& ids);
    
    /**
     * 从选择中移除
     */
    void removeFromSelection(EntityId id);
    void removeFromSelection(const std::vector<EntityId>& ids);
    
    /**
     * 切换选择状态
     */
    void toggleSelection(EntityId id);
    
    /**
     * 全选
     */
    void selectAll();
    
    /**
     * 反选
     */
    void invertSelection();

    // ============================================
    // 选择模式
    // ============================================
    
    enum class SelectMode {
        REPLACE,    // 替换选择（默认）
        ADD,        // 添加到选择（Shift）
        TOGGLE      // 切换选择（Ctrl）
    };
    
    /**
     * 根据模式选择
     */
    void selectWithMode(EntityId id, SelectMode mode);
    void selectWithMode(const std::vector<EntityId>& ids, SelectMode mode);

signals:
    /**
     * 选择变化信号
     * @param selectedCount 当前选中数量
     */
    void selectionChanged(int selectedCount);
    
    /**
     * 选中的实体列表变化
     */
    void selectedEntitiesChanged(const std::vector<EntityId>& ids);

private:
    /**
     * 更新文档中实体的选择状态
     */
    void syncToDocument();
    
    /**
     * 发出选择变化信号
     */
    void notifySelectionChanged();

private:
    Document* document_;                        // 文档引用
    std::unordered_set<EntityId> selectedIds_;  // 选中的实体 ID 集合
};