#include "RenderStyleManager.h"

RenderStyleManager::RenderStyleManager() {
    // 默认构造，使用initializeDefaultStyles()来初始化
}

StyleId RenderStyleManager::registerStyle(const QString& name, const RenderStyle& style) {
    StyleId id = nextStyleId_++;
    styles_[id] = style;
    nameToId_[name] = id;
    return id;
}

const RenderStyle* RenderStyleManager::getStyle(StyleId id) const {
    auto it = styles_.find(id);
    if (it != styles_.end()) {
        return &it.value();
    }
    return nullptr;
}

const RenderStyle* RenderStyleManager::getStyleByName(const QString& name) const {
    auto it = nameToId_.find(name);
    if (it != nameToId_.end()) {
        return getStyle(it.value());
    }
    return nullptr;
}

StyleId RenderStyleManager::getStyleId(const QString& name) const {
    auto it = nameToId_.find(name);
    if (it != nameToId_.end()) {
        return it.value();
    }
    return 0;
}

void RenderStyleManager::setStateStyleMapping(EntityState state, StyleId styleId) {
    stateToStyleId_[state] = styleId;
}

StyleId RenderStyleManager::getStyleIdForState(EntityState state) const {
    auto it = stateToStyleId_.find(state);
    if (it != stateToStyleId_.end()) {
        return it.value();
    }
    return 0; // 0表示无效
}

void RenderStyleManager::initializeDefaultStyles() {
    // 默认线样式
    StyleId normalLineId = registerStyle("line_normal", 
        RenderStyle("line_normal", ShaderType::LINE, QVector4D(1.0f, 1.0f, 1.0f, 1.0f)));
    StyleId hoveredLineId = registerStyle("line_hovered", 
        RenderStyle("line_hovered", ShaderType::LINE, QVector4D(1.0f, 1.0f, 0.0f, 1.0f)));
    StyleId selectedLineId = registerStyle("line_selected", 
        RenderStyle("line_selected", ShaderType::LINE, QVector4D(0.0f, 1.0f, 0.0f, 1.0f)));
    
    // 默认三角形样式
    StyleId normalTriId = registerStyle("triangle_normal", 
        RenderStyle("triangle_normal", ShaderType::TRIANGLE, QVector4D(0.8f, 0.8f, 0.8f, 1.0f)));
    StyleId hoveredTriId = registerStyle("triangle_hovered", 
        RenderStyle("triangle_hovered", ShaderType::TRIANGLE, QVector4D(1.0f, 1.0f, 0.0f, 1.0f)));
    StyleId selectedTriId = registerStyle("triangle_selected", 
        RenderStyle("triangle_selected", ShaderType::TRIANGLE, QVector4D(0.0f, 1.0f, 0.0f, 1.0f)));
    
    // 默认点样式
    StyleId normalPointId = registerStyle("point_normal", 
        RenderStyle("point_normal", ShaderType::POINT, QVector4D(1.0f, 1.0f, 1.0f, 1.0f)));
    StyleId hoveredPointId = registerStyle("point_hovered", 
        RenderStyle("point_hovered", ShaderType::POINT, QVector4D(1.0f, 1.0f, 0.0f, 1.0f)));
    StyleId selectedPointId = registerStyle("point_selected", 
        RenderStyle("point_selected", ShaderType::POINT, QVector4D(0.0f, 1.0f, 0.0f, 1.0f)));
    
    // 默认圆样式
    StyleId normalCircleId = registerStyle("circle_normal", 
        RenderStyle("circle_normal", ShaderType::CIRCLE, QVector4D(1.0f, 1.0f, 1.0f, 1.0f)));
    StyleId hoveredCircleId = registerStyle("circle_hovered", 
        RenderStyle("circle_hovered", ShaderType::CIRCLE, QVector4D(1.0f, 1.0f, 0.0f, 1.0f)));
    StyleId selectedCircleId = registerStyle("circle_selected", 
        RenderStyle("circle_selected", ShaderType::CIRCLE, QVector4D(0.0f, 1.0f, 0.0f, 1.0f)));
    
    // 设置状态映射（暂时使用line作为默认）
    // 实际使用时会根据entity的几何类型动态选择
    setStateStyleMapping(EntityState::NORMAL, normalLineId);
    setStateStyleMapping(EntityState::HOVERED, hoveredLineId);
    setStateStyleMapping(EntityState::SELECTED, selectedLineId);
}
