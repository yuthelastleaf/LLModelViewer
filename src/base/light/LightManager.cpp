#include "LightManager.h"
#include "../util/shader.h"
#include <algorithm>

// ============================================
// 光源创建方法
// ============================================

std::shared_ptr<Light> LightManager::createDirectionalLight() {
    auto light = std::make_shared<Light>(LightType::DIRECTIONAL);
    light->direction = glm::vec3(-0.2f, -1.0f, -0.3f);
    light->ambient = glm::vec3(0.2f);
    light->diffuse = glm::vec3(0.5f);
    light->specular = glm::vec3(1.0f);
    lights.push_back(light);
    return light;
}

std::shared_ptr<Light> LightManager::createPointLight(const glm::vec3& pos) {
    auto light = std::make_shared<Light>(LightType::POINT);
    light->position = pos;
    light->ambient = glm::vec3(0.2f);
    light->diffuse = glm::vec3(0.8f);
    light->specular = glm::vec3(1.0f);
    light->constant = 1.0f;
    light->linear = 0.09f;
    light->quadratic = 0.032f;
    lights.push_back(light);
    return light;
}

std::shared_ptr<Light> LightManager::createSpotLight(const glm::vec3& pos, const glm::vec3& dir) {
    auto light = std::make_shared<Light>(LightType::SPOT);
    light->position = pos;
    light->direction = glm::normalize(dir);
    light->ambient = glm::vec3(0.1f);
    light->diffuse = glm::vec3(1.0f);
    light->specular = glm::vec3(1.0f);
    light->constant = 1.0f;
    light->linear = 0.09f;
    light->quadratic = 0.032f;
    light->cutOff = glm::cos(glm::radians(12.5f));
    light->outerCutOff = glm::cos(glm::radians(15.0f));
    lights.push_back(light);
    return light;
}

// ============================================
// 光源管理方法
// ============================================

void LightManager::removeLight(std::shared_ptr<Light> light) {
    lights.erase(
        std::remove(lights.begin(), lights.end(), light), 
        lights.end()
    );
}

void LightManager::clear() {
    lights.clear();
}

// ============================================
// 着色器应用方法
// ============================================

void LightManager::applyLightsToShader(Shader& shader) const {
    shader.use();
    
    int dirLightCount = 0;
    int pointLightCount = 0;
    int spotLightCount = 0;
    
    for (const auto& light : lights) {
        if (!light->enabled) {
            continue;
        }
        
        std::string baseName;
        
        switch (light->type) {
            case LightType::DIRECTIONAL: {
                baseName = "dirLights[" + std::to_string(dirLightCount++) + "]";
                shader.setVec3(baseName + ".direction", light->direction);
                break;
            }
            
            case LightType::POINT: {
                baseName = "pointLights[" + std::to_string(pointLightCount++) + "]";
                shader.setVec3(baseName + ".position", light->position);
                shader.setFloat(baseName + ".constant", light->constant);
                shader.setFloat(baseName + ".linear", light->linear);
                shader.setFloat(baseName + ".quadratic", light->quadratic);
                break;
            }
            
            case LightType::SPOT: {
                baseName = "spotLights[" + std::to_string(spotLightCount++) + "]";
                shader.setVec3(baseName + ".position", light->position);
                shader.setVec3(baseName + ".direction", light->direction);
                shader.setFloat(baseName + ".cutOff", light->cutOff);
                shader.setFloat(baseName + ".outerCutOff", light->outerCutOff);
                shader.setFloat(baseName + ".constant", light->constant);
                shader.setFloat(baseName + ".linear", light->linear);
                shader.setFloat(baseName + ".quadratic", light->quadratic);
                break;
            }
        }
        
        // 设置通用光照属性
        shader.setVec3(baseName + ".ambient", light->ambient);
        shader.setVec3(baseName + ".diffuse", light->diffuse);
        shader.setVec3(baseName + ".specular", light->specular);
    }
    
    // 设置光源数量
    shader.setInt("dirLightCount", dirLightCount);
    shader.setInt("pointLightCount", pointLightCount);
    shader.setInt("spotLightCount", spotLightCount);
}

// ============================================
// 便捷方法
// ============================================

void LightManager::createDefaultLighting() {
    clear();
    
    // 创建一个从右上方照射的平行光
    auto dirLight = createDirectionalLight();
    dirLight->direction = glm::normalize(glm::vec3(-0.2f, -1.0f, -0.3f));
    dirLight->ambient = glm::vec3(0.2f);
    dirLight->diffuse = glm::vec3(0.5f);
    dirLight->specular = glm::vec3(1.0f);
}

void LightManager::createThreePointLighting() {
    clear();
    
    // 主光源（Key Light）- 从右前方照射
    auto keyLight = createDirectionalLight();
    keyLight->direction = glm::normalize(glm::vec3(-0.5f, -0.7f, -0.3f));
    keyLight->ambient = glm::vec3(0.1f);
    keyLight->diffuse = glm::vec3(0.8f);
    keyLight->specular = glm::vec3(1.0f);
    
    // 补光（Fill Light）- 从左侧照射，强度较弱
    auto fillLight = createDirectionalLight();
    fillLight->direction = glm::normalize(glm::vec3(0.5f, -0.3f, -0.2f));
    fillLight->ambient = glm::vec3(0.05f);
    fillLight->diffuse = glm::vec3(0.3f);
    fillLight->specular = glm::vec3(0.2f);
    
    // 轮廓光（Rim Light）- 从后方照射
    auto rimLight = createDirectionalLight();
    rimLight->direction = glm::normalize(glm::vec3(0.0f, 0.2f, 1.0f));
    rimLight->ambient = glm::vec3(0.0f);
    rimLight->diffuse = glm::vec3(0.4f);
    rimLight->specular = glm::vec3(0.5f);
}