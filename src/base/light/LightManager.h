#ifndef LIGHTMANAGER_H
#define LIGHTMANAGER_H

#include <glm/glm.hpp>
#include <vector>
#include <memory>

// 前向声明
class Shader;

/**
 * 光源类型枚举
 */
enum class LightType {
    DIRECTIONAL,    // 平行光（如太阳光）
    POINT,          // 点光源（如灯泡）
    SPOT            // 聚光灯（如手电筒）
};

/**
 * 光源结构体
 * 包含所有类型光源的属性
 */
struct Light {
    LightType type;
    
    // 位置和方向
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
    
    // 光照颜色
    glm::vec3 ambient = glm::vec3(0.1f);   // 环境光
    glm::vec3 diffuse = glm::vec3(1.0f);   // 漫反射
    glm::vec3 specular = glm::vec3(1.0f);  // 镜面反射
    
    // 衰减参数（用于点光源和聚光灯）
    float constant = 1.0f;        // 常数项
    float linear = 0.09f;         // 一次项
    float quadratic = 0.032f;     // 二次项
    
    // 聚光灯参数
    float cutOff = glm::cos(glm::radians(12.5f));       // 内圆锥角
    float outerCutOff = glm::cos(glm::radians(15.0f));  // 外圆锥角
    
    // 开关
    bool enabled = true;
    
    // 构造函数
    explicit Light(LightType t) : type(t) {}
};

/**
 * 光照管理器
 * 负责创建、管理和应用多个光源到着色器
 */
class LightManager {
public:
    LightManager() = default;
    ~LightManager() = default;
    
    // ============================================
    // 光源创建方法
    // ============================================
    
    /**
     * 创建平行光
     * @return 返回新创建的光源指针
     */
    std::shared_ptr<Light> createDirectionalLight();
    
    /**
     * 创建点光源
     * @param pos 光源位置
     * @return 返回新创建的光源指针
     */
    std::shared_ptr<Light> createPointLight(const glm::vec3& pos = glm::vec3(0.0f));
    
    /**
     * 创建聚光灯
     * @param pos 光源位置
     * @param dir 光源方向
     * @return 返回新创建的光源指针
     */
    std::shared_ptr<Light> createSpotLight(
        const glm::vec3& pos = glm::vec3(0.0f), 
        const glm::vec3& dir = glm::vec3(0.0f, -1.0f, 0.0f)
    );
    
    // ============================================
    // 光源管理方法
    // ============================================
    
    /**
     * 移除指定光源
     * @param light 要移除的光源
     */
    void removeLight(std::shared_ptr<Light> light);
    
    /**
     * 清除所有光源
     */
    void clear();
    
    /**
     * 获取所有光源（只读）
     */
    const std::vector<std::shared_ptr<Light>>& getLights() const { 
        return lights; 
    }
    
    /**
     * 获取光源数量
     */
    size_t getLightCount() const { 
        return lights.size(); 
    }
    
    /**
     * 获取指定索引的光源
     */
    std::shared_ptr<Light> getLight(size_t index) {
        return (index < lights.size()) ? lights[index] : nullptr;
    }
    
    // ============================================
    // 着色器应用方法
    // ============================================
    
    /**
     * 将所有光源参数应用到着色器
     * 
     * 着色器中需要定义以下 uniform：
     * - dirLights[MAX_DIR_LIGHTS]
     * - pointLights[MAX_POINT_LIGHTS]
     * - spotLights[MAX_SPOT_LIGHTS]
     * - dirLightCount, pointLightCount, spotLightCount
     * 
     * @param shader 目标着色器
     */
    void applyLightsToShader(Shader& shader) const;
    
    // 兼容旧的命名方式
    void ApplyLightsToShader(Shader& shader) const {
        applyLightsToShader(shader);
    }
    
    // ============================================
    // 便捷方法
    // ============================================
    
    /**
     * 创建默认场景光照（一个平行光）
     */
    void createDefaultLighting();
    
    /**
     * 创建三点光照（常用于人物渲染）
     */
    void createThreePointLighting();

private:
    std::vector<std::shared_ptr<Light>> lights;
};

#endif // LIGHTMANAGER_H