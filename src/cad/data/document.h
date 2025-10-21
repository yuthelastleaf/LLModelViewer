#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <variant>
#include <glm/glm.hpp>

using EntityId = std::uint64_t;

enum class EntityType { Line, Polyline, Circle, Arc };

struct Style {
    std::uint32_t rgba = 0xFFFFFFFF; // RGBA 格式: 0xRRGGBBAA
    float lineWidth = 1.0f;          // v0.1 仅参考
    // TODO: layerId, linetype, etc.
    
    // 便捷构造
    static Style fromRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        return Style{(uint32_t(r) << 24) | (uint32_t(g) << 16) | (uint32_t(b) << 8) | a};
    }
};

struct Line      { glm::vec3 p0, p1; };
struct Polyline  { std::vector<glm::vec3> pts; bool closed = false; };
struct Circle    { glm::vec3 c; float r; };
struct Arc       { glm::vec3 c; float r; float a0, a1; /* 弧度 */ };

struct Entity {
    EntityId id{};
    EntityType type{};
    Style style{};
    std::variant<Line, Polyline, Circle, Arc> geom;
    bool visible = true;
    bool dirty = true;  // 标记是否需要重新上传到 GPU
};

class Document {
public:
    const Entity* get(EntityId id) const;
    Entity*       get(EntityId id);

    std::vector<const Entity*> all() const;  // 只读遍历
    std::vector<Entity*>       all();        // 可写遍历

    EntityId add(Entity e);
    bool     remove(EntityId id);
    void     clear();
    
    // 更新实体（标记为 dirty）
    bool update(EntityId id, const Entity& e);
    void markDirty(EntityId id);
    void clearAllDirtyFlags();

    // 更新实体信息
    bool updateEndLinePoint(EntityId id, glm::vec3 linepos);

    // 便捷构造（可选）
    EntityId addLine(const glm::vec3& a, const glm::vec3& b, const Style& s = {});
    EntityId addPolyline(const std::vector<glm::vec3>& pts, bool closed, const Style& s = {});
    EntityId addCircle(const glm::vec3& c, float r, const Style& s = {});
    EntityId addArc(const glm::vec3& c, float r, float a0, float a1, const Style& s = {});

private:
    std::unordered_map<EntityId, Entity> map_;
    EntityId next_ = 1;
};