#include "document.h"
#include <limits>

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
    auto it = map_.find(id);
    if (it == map_.end()) return false;
    map_.erase(it);
    if (onRemove_) onRemove_(id);  // 通知监听者
    return true;
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

// ❌ 已废弃：selected/hovered标志已移除，由SelectionSystem管理
void Document::clearAllSelectedFlags()
{
    // No-op: selected flags removed from Entity
}

// ❌ 已废弃：selected/hovered标志已移除，由SelectionSystem管理
void Document::clearAllHoverFlags()
{
    // No-op: hovered flags removed from Entity
}

// ❌ 已废弃：selected/hovered标志已移除，由SelectionSystem管理
void Document::transHoverToSelected()
{
    // No-op: state management now handled by SelectionSystem
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
    } else if(it->second.type == EntityType::Rectangle) {
        if (auto* rect = std::get_if<Rectangle>(&it->second.geom)) {
            rect->p1 = linepos;
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

EntityId Document::addRectangle(const glm::vec3 &a, const glm::vec3 &b, bool closed, const Style &s, bool doted)
{
    Entity e;
    e.type = EntityType::Rectangle;
    e.style = s;
    e.geom = Rectangle{a, b};
    e.dot = doted;
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

EntityId Document::addGizmoAxis(const glm::vec3& origin, const glm::vec3& direction, 
                                float length, int axisIndex, const Style& s)
{
    if (length <= 0.0f) return 0;  // 边界检查
    
    Entity e;
    e.type = EntityType::GizmoAxis;
    e.style = s;
    e.geom = GizmoAxis{origin, direction, length, axisIndex};
    e.isGizmo = true;  // 标记为 Gizmo
    return add(std::move(e));
}

// ============================================
// ✅ v0.4: 命令系统支持方法
// ============================================

std::unique_ptr<Entity> Document::removeAndTake(EntityId id) {
    auto it = map_.find(id);
    if (it == map_.end()) {
        return nullptr;
    }
    
    // 复制实体到智能指针
    auto entity = std::make_unique<Entity>(it->second);
    
    // 通知删除回调
    if (onRemove_) {
        onRemove_(id);
    }
    
    // 从映射中删除
    map_.erase(it);
    
    return entity;
}

EntityId Document::addWithId(EntityId id, std::unique_ptr<Entity> e) {
    if (!e || map_.find(id) != map_.end()) {
        return 0; // ID已存在或实体为空
    }
    
    e->id = id;
    e->dirty = true;  // 恢复的实体标记为脏，需要重新渲染
    
    // 更新next_以避免ID冲突
    if (id >= next_) {
        next_ = id + 1;
    }
    
    EntityId resultId = e->id;
    map_[id] = *e;  // 复制实体内容
    
    return resultId;
}

void Document::notifyEntityChanged(EntityId id) {
    auto it = map_.find(id);
    if (it != map_.end()) {
        it->second.dirty = true;  // 标记需要重新上传到GPU
    }
}

void Document::getBoundingBox(glm::vec3& minBound, glm::vec3& maxBound) const {
    minBound = glm::vec3(std::numeric_limits<float>::max());
    maxBound = glm::vec3(std::numeric_limits<float>::lowest());
    
    bool hasEntities = false;
    
    for (const auto& [id, entity] : map_) {
        if (!entity.visible || entity.isGizmo) continue;
        
        hasEntities = true;
        
        // 根据实体类型获取点
        std::visit([&](const auto& geom) {
            using T = std::decay_t<decltype(geom)>;
            
            if constexpr (std::is_same_v<T, Line>) {
                minBound = glm::min(minBound, geom.p0);
                minBound = glm::min(minBound, geom.p1);
                maxBound = glm::max(maxBound, geom.p0);
                maxBound = glm::max(maxBound, geom.p1);
            }
            else if constexpr (std::is_same_v<T, Polyline>) {
                for (const auto& pt : geom.pts) {
                    minBound = glm::min(minBound, pt);
                    maxBound = glm::max(maxBound, pt);
                }
            }
            else if constexpr (std::is_same_v<T, Rectangle>) {
                minBound = glm::min(minBound, geom.p0);
                minBound = glm::min(minBound, geom.p1);
                maxBound = glm::max(maxBound, geom.p0);
                maxBound = glm::max(maxBound, geom.p1);
            }
            else if constexpr (std::is_same_v<T, Circle>) {
                glm::vec3 rVec(geom.r, geom.r, 0.0f);
                minBound = glm::min(minBound, geom.c - rVec);
                maxBound = glm::max(maxBound, geom.c + rVec);
            }
            else if constexpr (std::is_same_v<T, Arc>) {
                glm::vec3 rVec(geom.r, geom.r, 0.0f);
                minBound = glm::min(minBound, geom.c - rVec);
                maxBound = glm::max(maxBound, geom.c + rVec);
            }
            else if constexpr (std::is_same_v<T, Box>) {
                glm::vec3 halfSize(geom.size * 0.5f);
                minBound = glm::min(minBound, geom.center - halfSize);
                maxBound = glm::max(maxBound, geom.center + halfSize);
            }
        }, entity.geom);
    }
    
    // 如果没有实体，返回原点周围的默认范围
    if (!hasEntities) {
        minBound = glm::vec3(-10.0f);
        maxBound = glm::vec3(10.0f);
    }
}
