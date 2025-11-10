#ifndef RENDERSTYLEMANAGER_H
#define RENDERSTYLEMANAGER_H

#include <QVector3D>
#include <QVector4D>
#include <QString>
#include <QMap>
#include <memory>

enum class ShaderType {
    LINE,
    TRIANGLE,
    POINT,
    CIRCLE,
    // 未来可扩展
    PHONG_3D,
    PBR_3D
};

enum class EntityState {
    NORMAL,
    HOVERED,
    SELECTED
};

using StyleId = int;

struct RenderStyle {
    QString name;
    ShaderType shaderType;
    QVector4D color;
    // lineWidth removed - not supported in your environment
    
    RenderStyle() 
        : shaderType(ShaderType::LINE)
        , color(1.0f, 1.0f, 1.0f, 1.0f) {}
    
    RenderStyle(const QString& n, ShaderType type, const QVector4D& col)
        : name(n), shaderType(type), color(col) {}
};

class RenderStyleManager {
public:
    static RenderStyleManager& instance() {
        static RenderStyleManager inst;
        return inst;
    }
    
    // 注册样式，返回StyleId
    StyleId registerStyle(const QString& name, const RenderStyle& style);
    
    // 查询样式
    const RenderStyle* getStyle(StyleId id) const;
    const RenderStyle* getStyleByName(const QString& name) const;
    StyleId getStyleId(const QString& name) const;  // 新增：直接获取StyleId
    
    // 状态到样式的映射
    void setStateStyleMapping(EntityState state, StyleId styleId);
    StyleId getStyleIdForState(EntityState state) const;
    
    // 初始化默认样式
    void initializeDefaultStyles();
    
    // 未来可扩展：从文件加载样式配置
    // bool loadStylesFromFile(const QString& filePath);
    
private:
    RenderStyleManager();
    ~RenderStyleManager() = default;
    RenderStyleManager(const RenderStyleManager&) = delete;
    RenderStyleManager& operator=(const RenderStyleManager&) = delete;
    
    StyleId nextStyleId_ = 1;
    QMap<StyleId, RenderStyle> styles_;
    QMap<QString, StyleId> nameToId_;
    QMap<EntityState, StyleId> stateToStyleId_;
};

#endif // RENDERSTYLEMANAGER_H
