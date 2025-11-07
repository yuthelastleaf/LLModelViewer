#pragma once
#include "../data/document.h"
#include <glm/glm.hpp>
#include <optional>

/**
 * Transform - 变换工具类
 * 
 * 功能：
 * - 计算实体的中心点
 * - 应用变换（平移、旋转、缩放）
 * - 提供变换辅助方法
 */
class Transform {
public:
    /**
     * 计算实体的中心点
     * @param entity 实体指针
     * @return 中心点坐标，如果无法计算则返回 nullopt
     */
    static std::optional<glm::vec3> getEntityCenter(const Entity* entity);
    
    /**
     * 计算多个实体的整体中心点（平均值）
     * @param entities 实体列表
     * @return 整体中心点
     */
    static glm::vec3 getSelectionCenter(const std::vector<Entity*>& entities);
    
    /**
     * 平移实体
     * @param entity 实体指针
     * @param offset 平移偏移量
     */
    static void translateEntity(Entity* entity, const glm::vec3& offset);
    
    /**
     * 平移多个实体
     * @param entities 实体列表
     * @param offset 平移偏移量
     */
    static void translateEntities(const std::vector<Entity*>& entities, const glm::vec3& offset);

    /**
     * 计算实体的包围盒
     * @param entity 实体指针
     * @param outMin [输出] 包围盒最小点
     * @param outMax [输出] 包围盒最大点
     * @return 是否成功计算
     */
    static bool getEntityBounds(const Entity* entity, glm::vec3& outMin, glm::vec3& outMax);

private:
    // 辅助方法：计算点集的平均值
    static glm::vec3 computeAverage(const std::vector<glm::vec3>& points);
};
