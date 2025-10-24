#include "renderer.h"
#include <cmath>

bool Renderer::initialize()
{
    initializeOpenGLFunctions();

    try
    {
        // ✅ 使用自定义 Shader 类
        shaderLines_ = std::make_unique<Shader>(
            "shaders/cadshaders/line/line.vs",
            "shaders/cadshaders/line/line.fs");

        if (!shaderLines_ || shaderLines_->ID == 0)
        {
            qCritical() << "Failed to create shader program";
            return false;
        }
        qDebug() << "Renderer initialized successfully with custom Shader";
        qDebug() << "Shader ID:" << shaderLines_->ID;

        return true;
    }
    catch (const std::exception &e)
    {
        qCritical() << "Failed to initialize renderer:" << e.what();
        return false;
    }
}

void Renderer::shutdown()
{
    // 清理所有批次
    for (auto &kv : batches_)
    {
        freeBatch_(kv.second);
    }
    batches_.clear();

    // ✅ Shader 通过 unique_ptr 自动清理
    shaderLines_.reset();

    qDebug() << "Renderer shutdown complete";
}
GLuint Renderer::makeVao(GLuint vbo, GLuint ibo)
{
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PosVertex), (void *)0);
    glEnableVertexAttribArray(0);
    if (ibo)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    }
    glBindVertexArray(0);
    return vao;
}

void Renderer::freeBatch_(GpuBatch &b)
{
    if (b.ibo)
        glDeleteBuffers(1, &b.ibo), b.ibo = 0;
    if (b.vbo)
        glDeleteBuffers(1, &b.vbo), b.vbo = 0;
    if (b.vao)
        glDeleteVertexArrays(1, &b.vao), b.vao = 0;
    b.indexCount = 0;
}

// 修正：RGBA 格式 0xRRGGBBAA
static glm::vec4 rgbaToVec4(std::uint32_t rgba)
{
    float r = ((rgba >> 24) & 0xFF) / 255.0f;
    float g = ((rgba >> 16) & 0xFF) / 255.0f;
    float b = ((rgba >> 8) & 0xFF) / 255.0f;
    float a = ((rgba >> 0) & 0xFF) / 255.0f;
    return {r, g, b, a};
}

void ViewportState::updateWorldPerPixel()
{
    if (width <= 0 || height <= 0)
    {
        worldPerPixel = 1.0f;
        return;
    }
    // 简化计算：取视图矩阵的缩放因子倒数
    glm::vec4 origin = view * glm::vec4(0, 0, 0, 1);
    glm::vec4 offset = view * glm::vec4(1, 0, 0, 1);
    float viewScale = glm::length(glm::vec3(offset - origin));

    // 透视投影的近似值
    float ndcWidth = 2.0f; // NDC 空间宽度
    worldPerPixel = ndcWidth / (viewScale * float(width));
}

// ============================================
// 屏幕坐标转世界坐标
// ============================================
glm::vec3 ViewportState::screenToWorld(int screenX, int screenY, float planeZ) const
{
    // 1. 屏幕坐标 → NDC（标准化设备坐标）
    float ndcX = (2.0f * screenX) / width - 1.0f;
    float ndcY = 1.0f - (2.0f * screenY) / height; // Y 轴翻转

    return ndcToWorld(ndcX, ndcY, planeZ);
}

glm::vec3 ViewportState::screenToWorld(const QPoint &screenPos, float planeZ) const
{
    return screenToWorld(screenPos.x(), screenPos.y(), planeZ);
}

// ============================================
// 世界坐标转屏幕坐标
// ============================================
glm::vec2 ViewportState::worldToScreen(const glm::vec3 &worldPos) const
{
    // 1. 世界坐标 → 裁剪空间
    glm::vec4 clipPos = proj * view * glm::vec4(worldPos, 1.0f);

    // 2. 透视除法 → NDC
    if (std::abs(clipPos.w) < 1e-6f)
    {
        // 防止除零
        return glm::vec2(-1.0f, -1.0f); // 无效坐标
    }

    glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;

    // 3. NDC → 屏幕坐标
    float screenX = (ndc.x + 1.0f) * 0.5f * width;
    float screenY = (1.0f - ndc.y) * 0.5f * height; // Y 轴翻转

    return glm::vec2(screenX, screenY);
}

// ============================================
// NDC 转世界坐标
// ============================================
glm::vec3 ViewportState::ndcToWorld(float ndcX, float ndcY, float planeZ) const
{
    // 1. 计算 view-projection 的逆矩阵
    glm::mat4 invVP = glm::inverse(proj * view);

    // 2. 构造射线（从近平面到远平面）
    glm::vec4 rayStart_clip = glm::vec4(ndcX, ndcY, -1.0f, 1.0f); // 近平面（z=-1）
    glm::vec4 rayEnd_clip = glm::vec4(ndcX, ndcY, 1.0f, 1.0f);    // 远平面（z=1）

    // 3. 转换到世界空间
    glm::vec4 rayStart_world = invVP * rayStart_clip;
    glm::vec4 rayEnd_world = invVP * rayEnd_clip;

    // 4. 透视除法
    rayStart_world /= rayStart_world.w;
    rayEnd_world /= rayEnd_world.w;

    // 5. 计算射线
    glm::vec3 rayOrigin = glm::vec3(rayStart_world);
    glm::vec3 rayDir = glm::normalize(glm::vec3(rayEnd_world - rayStart_world));

    // 6. 求射线与 z=planeZ 平面的交点
    // 平面方程: z = planeZ
    // 射线方程: P = rayOrigin + t * rayDir
    // 解方程: rayOrigin.z + t * rayDir.z = planeZ

    if (std::abs(rayDir.z) < 1e-6f)
    {
        // 射线与平面平行，直接投影到平面上
        return glm::vec3(rayOrigin.x, rayOrigin.y, planeZ);
    }

    float t = (planeZ - rayOrigin.z) / rayDir.z;
    glm::vec3 intersection = rayOrigin + t * rayDir;

    return intersection;
}

// ============================================
// 世界坐标转 NDC
// ============================================
glm::vec3 ViewportState::worldToNDC(const glm::vec3 &worldPos) const
{
    // 1. 世界坐标 → 裁剪空间
    glm::vec4 clipPos = proj * view * glm::vec4(worldPos, 1.0f);

    // 2. 透视除法 → NDC
    if (std::abs(clipPos.w) < 1e-6f)
    {
        return glm::vec3(0.0f, 0.0f, 0.0f); // 无效坐标
    }

    glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;

    return ndc;
}

// ============================================
// 判断点是否在视野内
// ============================================
bool ViewportState::isVisible(const glm::vec3 &worldPos) const
{
    // 转换到 NDC
    glm::vec3 ndc = worldToNDC(worldPos);

    // NDC 范围：[-1, 1]
    // 检查是否在视锥体内
    bool inX = (ndc.x >= -1.0f && ndc.x <= 1.0f);
    bool inY = (ndc.y >= -1.0f && ndc.y <= 1.0f);
    bool inZ = (ndc.z >= -1.0f && ndc.z <= 1.0f);

    return inX && inY && inZ;
}

// ============================================
// 获取世界空间中某点的像素大小
// ============================================
float ViewportState::getPixelSizeAt(const glm::vec3 &worldPos) const
{
    // 在世界空间中，计算相邻一个像素的距离

    // 1. 世界点转屏幕坐标
    glm::vec2 screenPos = worldToScreen(worldPos);

    // 2. 屏幕坐标偏移 1 像素
    glm::vec3 worldPos1 = screenToWorld((int)screenPos.x + 1, (int)screenPos.y, worldPos.z);

    // 3. 计算世界空间距离
    float pixelSize = glm::distance(worldPos, worldPos1);

    return pixelSize;
}

// ============================================
// 获取视锥体的 8 个角点
// ============================================
void ViewportState::getFrustumCorners(glm::vec3 corners[8]) const
{
    // NDC 空间的 8 个角点
    // 近平面 (z = -1): 左下、右下、右上、左上
    // 远平面 (z = 1):  左下、右下、右上、左上

    glm::vec3 ndcCorners[8] = {
        // 近平面 (z = -1)
        {-1.0f, -1.0f, -1.0f}, // 左下
        {1.0f, -1.0f, -1.0f},  // 右下
        {1.0f, 1.0f, -1.0f},   // 右上
        {-1.0f, 1.0f, -1.0f},  // 左上
        // 远平面 (z = 1)
        {-1.0f, -1.0f, 1.0f}, // 左下
        {1.0f, -1.0f, 1.0f},  // 右下
        {1.0f, 1.0f, 1.0f},   // 右上
        {-1.0f, 1.0f, 1.0f}   // 左上
    };

    // 转换到世界空间
    glm::mat4 invVP = glm::inverse(proj * view);

    for (int i = 0; i < 8; ++i)
    {
        glm::vec4 worldCorner = invVP * glm::vec4(ndcCorners[i], 1.0f);
        worldCorner /= worldCorner.w; // 透视除法
        corners[i] = glm::vec3(worldCorner);
    }
}

void Renderer::syncFromDocument(const Document &doc, const ViewportState &vp, bool forceRebuild)
{
    // 检查是否需要因缩放级别变化而重新细分圆弧
    bool needRetessellate = forceRebuild ||
                            (std::abs(vp.worldPerPixel - lastWorldPerPixel_) / vp.worldPerPixel > 0.5f);

    if (needRetessellate)
    {
        lastWorldPerPixel_ = vp.worldPerPixel;
    }

    if (forceRebuild)
    {
        // 全量重建
        for (auto &kv : batches_)
            freeBatch_(kv.second);
        batches_.clear();
    }

    for (auto *e : doc.all())
    {
        if (!e->visible)
        {
            // 隐藏的实体：移除批次
            if (batches_.count(e->id))
            {
                freeBatch_(batches_[e->id]);
                batches_.erase(e->id);
            }
            continue;
        }

        // 只处理脏实体或需要重新细分的圆弧
        bool needUpdate = e->dirty;
        if (!needUpdate && needRetessellate)
        {
            needUpdate = (e->type == EntityType::Circle || e->type == EntityType::Arc);
        }

        if (!needUpdate && batches_.count(e->id))
        {
            continue; // 已有批次且无需更新
        }

        // 删除旧批次
        if (batches_.count(e->id))
        {
            freeBatch_(batches_[e->id]);
            batches_.erase(e->id);
        }

        // 上传新批次
        switch (e->type)
        {
        case EntityType::Line:
        {
            uploadLine_(e->id, std::get<Line>(e->geom), e->style.rgba);
        }
        break;
        case EntityType::Polyline:
        {
            uploadPolyline_(e->id, std::get<Polyline>(e->geom), e->style.rgba);
        }
        break;
        case EntityType::Circle:
        {
            uploadCircle_(e->id, std::get<Circle>(e->geom), e->style.rgba, vp);
        }
        break;
        case EntityType::Arc:
        {
            uploadArc_(e->id, std::get<Arc>(e->geom), e->style.rgba, vp);
        }
        break;
        case EntityType::Box:
        {
            uploadBox_(e->id, std::get<Box>(e->geom), e->style.rgba);
        }
        break;
        }
    }
}

void Renderer::removeBatch(EntityId id)
{
    auto it = batches_.find(id);
    if (it != batches_.end())
    {
        freeBatch_(it->second);
        batches_.erase(it);
    }
}

void Renderer::draw(const ViewportState &vp)
{
    if (!shaderLines_)
    {
        qWarning() << "Shader not initialized";
        return;
    }

    // 激活着色器
    shaderLines_->use();

    // 检查当前程序
    GLint currentProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);

    // 设置 MVP
    glm::mat4 model(1.0f);
    glm::mat4 mvp = vp.proj * vp.view * model;
    shaderLines_->setMat4("mvp", mvp);

    // 绘制所有批次
    int batchIndex = 0;
    for (const auto &kv : batches_)
    {
        const GpuBatch &batch = kv.second;

        if (batch.indexCount == 0)
        {
            continue;
        }

        if (batch.vao == 0)
        {
            continue;
        }

        // 设置颜色
        float r = ((batch.rgba >> 24) & 0xFF) / 255.0f;
        float g = ((batch.rgba >> 16) & 0xFF) / 255.0f;
        float b = ((batch.rgba >> 8) & 0xFF) / 255.0f;
        float a = ((batch.rgba) & 0xFF) / 255.0f;

        shaderLines_->setVec4("color", glm::vec4(r, g, b, a));
        glBindVertexArray(batch.vao);

        // 验证绑定
        GLint vaoBinding;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vaoBinding);

        // 检查 IBO
        GLint iboBinding;
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &iboBinding);
        if (batch.ibo != 0)
        {
            glDrawElements(batch.drawMode, batch.indexCount, GL_UNSIGNED_INT, nullptr);
        }
        else
        {
            glDrawArrays(batch.drawMode, 0, batch.indexCount);
        }
        glBindVertexArray(0);
    }
}

void Renderer::drawLineStrip(const std::vector<glm::vec3> &pts,
                             std::uint32_t rgba,
                             const ViewportState &vp)
{
    if (pts.empty() || !shaderLines_)
    {
        return;
    }

    // 转换顶点
    std::vector<PosVertex> vertices;
    vertices.reserve(pts.size());
    for (const auto &p : pts)
    {
        vertices.push_back({p});
    }

    // 创建临时 VAO/VBO
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(PosVertex),
                 vertices.data(), GL_STREAM_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PosVertex), (void *)0);
    glEnableVertexAttribArray(0);

    // ✅ 使用着色器绘制
    shaderLines_->use();

    glm::mat4 model(1.0f);
    glm::mat4 mvp = vp.proj * vp.view * model;
    shaderLines_->setMat4("mvp", mvp);

    float r = ((rgba >> 24) & 0xFF) / 255.0f;
    float g = ((rgba >> 16) & 0xFF) / 255.0f;
    float b = ((rgba >> 8) & 0xFF) / 255.0f;
    float a = ((rgba) & 0xFF) / 255.0f;
    shaderLines_->setVec4("color", glm::vec4(r, g, b, a));

    glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(vertices.size()));

    // 清理
    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void Renderer::drawLineSegments(const std::vector<glm::vec3> &ptsPairs,
                                std::uint32_t rgba,
                                const ViewportState &vp)
{
    if (ptsPairs.empty() || !shaderLines_)
    {
        return;
    }

    std::vector<PosVertex> vertices;
    vertices.reserve(ptsPairs.size());
    for (const auto &p : ptsPairs)
    {
        vertices.push_back({p});
    }

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(PosVertex),
                 vertices.data(), GL_STREAM_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PosVertex), (void *)0);
    glEnableVertexAttribArray(0);

    // ✅ 使用着色器绘制
    shaderLines_->use();

    glm::mat4 model(1.0f);
    glm::mat4 mvp = vp.proj * vp.view * model;
    shaderLines_->setMat4("mvp", mvp);

    float r = ((rgba >> 24) & 0xFF) / 255.0f;
    float g = ((rgba >> 16) & 0xFF) / 255.0f;
    float b = ((rgba >> 8) & 0xFF) / 255.0f;
    float a = ((rgba) & 0xFF) / 255.0f;
    shaderLines_->setVec4("color", glm::vec4(r, g, b, a));

    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertices.size()));

    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

// ========== 上传实体 ==========
void Renderer::uploadLine_(EntityId id, const Line &L, std::uint32_t rgba)
{
    GpuBatch b{};
    std::vector<PosVertex> vb = {{L.p0}, {L.p1}};
    glGenBuffers(1, &b.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, b.vbo);
    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(vb.size() * sizeof(PosVertex)), vb.data(), GL_STATIC_DRAW);
    b.vao = makeVao(b.vbo, 0);
    b.indexCount = GLsizei(vb.size());
    b.rgba = rgba;
    b.drawMode = GL_LINES;
    batches_[id] = b;
}

void Renderer::uploadPolyline_(EntityId id, const Polyline &P, std::uint32_t rgba)
{
    if (P.pts.size() < 2)
        return;
    GpuBatch b{};
    std::vector<PosVertex> vb(P.pts.size() + (P.closed ? 1 : 0));
    for (size_t i = 0; i < P.pts.size(); ++i)
        vb[i].pos = P.pts[i];
    if (P.closed)
        vb.back().pos = P.pts.front();

    glGenBuffers(1, &b.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, b.vbo);
    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(vb.size() * sizeof(PosVertex)), vb.data(), GL_STATIC_DRAW);
    b.vao = makeVao(b.vbo, 0);
    b.indexCount = GLsizei(vb.size());
    b.rgba = rgba;
    b.drawMode = GL_LINE_STRIP;
    batches_[id] = b;
}

void Renderer::uploadCircle_(EntityId id, const Circle &C, std::uint32_t rgba, const ViewportState &vp)
{
    float worldEps = vp.worldPerPixel * 0.5f;
    auto pts = tessellateCircle(C, worldEps);
    Polyline P{pts, true};
    uploadPolyline_(id, P, rgba);
}

void Renderer::uploadArc_(EntityId id, const Arc &A, std::uint32_t rgba, const ViewportState &vp)
{
    float worldEps = vp.worldPerPixel * 0.5f;
    auto pts = tessellateArc(A, worldEps);
    Polyline P{pts, false};
    uploadPolyline_(id, P, rgba);
}

void Renderer::uploadBox_(EntityId id, const Box &B, std::uint32_t rgba)
{
    // ✅ 修改为实心立方体（可选）
    float half = B.size * 0.5f;
    glm::vec3 c = B.center;

    // 8 个顶点
    glm::vec3 v[8] = {
        c + glm::vec3(-half, -half, -half), // 0
        c + glm::vec3(half, -half, -half),  // 1
        c + glm::vec3(half, half, -half),   // 2
        c + glm::vec3(-half, half, -half),  // 3
        c + glm::vec3(-half, -half, half),  // 4
        c + glm::vec3(half, -half, half),   // 5
        c + glm::vec3(half, half, half),    // 6
        c + glm::vec3(-half, half, half),   // 7
    };

    std::vector<PosVertex> vertices;
    std::vector<GLuint> indices;

    // ✅ 实心模式：6 个面，12 个三角形
    for (int i = 0; i < 8; ++i)
    {
        vertices.push_back({v[i]});
    }

    indices = {
        // 后面 (-Z)
        0, 1, 2, 2, 3, 0,
        // 前面 (+Z)
        4, 6, 5, 4, 7, 6,
        // 左面 (-X)
        0, 3, 7, 0, 7, 4,
        // 右面 (+X)
        1, 5, 6, 1, 6, 2,
        // 底面 (-Y)
        0, 4, 5, 0, 5, 1,
        // 顶面 (+Y)
        3, 2, 6, 3, 6, 7};

    // 上传到 GPU
    removeBatch(id);

    GpuBatch batch;
    glGenBuffers(1, &batch.vbo);
    glGenBuffers(1, &batch.ibo);

    glBindBuffer(GL_ARRAY_BUFFER, batch.vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(PosVertex),
                 vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(GLuint),
                 indices.data(), GL_STATIC_DRAW);

    batch.vao = makeVao(batch.vbo, batch.ibo);
    batch.indexCount = static_cast<GLsizei>(indices.size());
    batch.rgba = rgba;
    batch.drawMode = GL_TRIANGLES; // ✅ 实心模式

    batches_[id] = batch;
}

// ========== 细分：保证弧边弦高误差约 <= worldEps ==========
static int segsForRadius(float r, float worldEps, float spanRadians)
{
    // chord error e ≈ r * (1 - cos(theta/2)) <= worldEps
    // theta_max ≈ 2 * acos(1 - e/r)
    if (r < 1e-6f)
        return 8;
    float thetaMax = 2.0f * std::acos(std::max(0.0f, 1.0f - worldEps / r));
    if (thetaMax <= 0.0f)
        thetaMax = 0.1f; // fallback
    int n = int(std::ceil(spanRadians / thetaMax));
    return std::max(8, std::min(n, 360)); // 限制在 [8, 360]
}

std::vector<glm::vec3> Renderer::tessellateCircle(const Circle &C, float worldEps)
{
    int n = segsForRadius(C.r, worldEps, 2.0f * float(M_PI));
    std::vector<glm::vec3> pts;
    pts.reserve(n);
    for (int i = 0; i < n; i++)
    {
        float t = (float(i) / float(n)) * 2.0f * float(M_PI);
        pts.push_back({C.c.x + C.r * std::cos(t), C.c.y + C.r * std::sin(t), C.c.z});
    }
    return pts;
}

std::vector<glm::vec3> Renderer::tessellateArc(const Arc &A, float worldEps)
{
    float span = A.a1 - A.a0;
    // 归一化到 [0, 2pi]
    while (span < 0)
        span += 2.0f * float(M_PI);
    while (span > 2.0f * float(M_PI))
        span -= 2.0f * float(M_PI);

    int n = segsForRadius(A.r, worldEps, span);
    n = std::max(2, n);
    std::vector<glm::vec3> pts;
    pts.reserve(n + 1);
    for (int i = 0; i <= n; i++)
    {
        float t = A.a0 + span * (float(i) / float(n));
        pts.push_back({A.c.x + A.r * std::cos(t), A.c.y + A.r * std::sin(t), A.c.z});
    }
    return pts;
}