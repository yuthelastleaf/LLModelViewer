#include "document.h"

const Entity* Document::get(EntityId id) const {
    auto it = map_.find(id);
    return it == map_.end() ? nullptr : &it->second;
}
Entity* Document::get(EntityId id) {
    auto it = map_.find(id);
    return it == map_.end() ? nullptr : &it->second;
}

std::vector<const Entity*> Document::all() const {
    std::vector<const Entity*> out;
    out.reserve(map_.size());
    for (auto& kv : map_) out.push_back(&kv.second);
    return out;
}
std::vector<Entity*> Document::all() {
    std::vector<Entity*> out;
    out.reserve(map_.size());
    for (auto& kv : map_) out.push_back(&kv.second);
    return out;
}

EntityId Document::add(Entity e) {
    if (e.id == 0) e.id = next_++;
    e.dirty = true;  // 新实体标记为脏
    map_[e.id] = std::move(e);
    return e.id;
}

bool Document::remove(EntityId id) {
    return map_.erase(id) > 0;
}

void Document::clear() {
    map_.clear();
    next_ = 1;
}

bool Document::update(EntityId id, const Entity& e) {
    auto it = map_.find(id);
    if (it == map_.end()) return false;
    it->second = e;
    it->second.id = id;  // 保持 ID 不变
    it->second.dirty = true;
    return true;
}

void Document::markDirty(EntityId id) {
    auto it = map_.find(id);
    if (it != map_.end()) {
        it->second.dirty = true;
    }
}

void Document::clearAllDirtyFlags() {
    for (auto& kv : map_) {
        kv.second.dirty = false;
    }
}

bool Document::updateEndLinePoint(EntityId id, glm::vec3 linepos)
{
    bool flag = false;
    auto it = map_.find(id);
    if (it == map_.end()) {
        return flag;
    }
    if(it->second.type == EntityType::Line) { // 保持 ID 不变
        // ✅ 推荐：检查类型后修改
        if (auto* line = std::get_if<Line>(&it->second.geom)) {
            line->p1 = linepos;
        }
        it->second.dirty = true;
        flag = true;
    }
    return flag;
}

EntityId Document::addLine(const glm::vec3& a, const glm::vec3& b, const Style& s) {
    Entity e;
    e.type = EntityType::Line;
    e.style = s;
    e.geom = Line{a,b};
    return add(std::move(e));
}

EntityId Document::addPolyline(const std::vector<glm::vec3>& pts, bool closed, const Style& s) {
    if (pts.size() < 2) return 0;  // 边界检查
    Entity e;
    e.type = EntityType::Polyline;
    e.style = s;
    e.geom = Polyline{pts, closed};
    return add(std::move(e));
}

EntityId Document::addCircle(const glm::vec3& c, float r, const Style& s) {
    if (r <= 0.0f) return 0;  // 边界检查
    Entity e;
    e.type = EntityType::Circle;
    e.style = s;
    e.geom = Circle{c, r};
    return add(std::move(e));
}

EntityId Document::addArc(const glm::vec3& c, float r, float a0, float a1, const Style& s) {
    if (r <= 0.0f) return 0;  // 边界检查
    Entity e;
    e.type = EntityType::Arc;
    e.style = s;
    e.geom = Arc{c, r, a0, a1};
    return add(std::move(e));
}

EntityId Document::addBox(const glm::vec3 &center, float size, const Style &s)
{
    if (size <= 0.0f) return 0;  // 边界检查
    
    Entity e;
    e.type = EntityType::Box;
    e.style = s;
    e.geom = Box{center, size};
    return add(std::move(e));
}
