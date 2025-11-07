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
            qCritical() << "Failed to create shaderLines_";
            return false;
        }

        shader_hover_Lines_ = std::make_unique<Shader>(
            "shaders/cadshaders/line_hover/line_hover.vs",
            "shaders/cadshaders/line_hover/line_hover.gs",
            "shaders/cadshaders/line_hover/line_hover.fs");
        if (!shader_hover_Lines_ || shader_hover_Lines_->ID == 0)
        {
            qCritical() << "Failed to create shader_hover_Lines_";
            return false;
        }

        shader_hover_Solid_ = std::make_unique<Shader>(
            "shaders/cadshaders/solid_hover/solid_hover.vs",
            "shaders/cadshaders/solid_hover/solid_hover.fs");

        if (!shader_hover_Solid_ || shader_hover_Solid_->ID == 0)
        {
            qCritical() << "Failed to create shader_hover_Solid_";
            return false;
        }

        shaderDottedLines_ = std::make_unique<Shader>(
            "shaders/cadshaders/line_dotted/line_dotted.vs",
            "shaders/cadshaders/line_dotted/line_dotted.fs");

        if (!shaderDottedLines_ || shaderDottedLines_->ID == 0)
        {
            qCritical() << "Failed to create shaderDottedLines_";
            return false;
        }

        // ✅ 手动设置测试值
        hoverStyle_.color = 0x00FF00FF; // 绿色（更明显）
        hoverStyle_.lineWidth = 3.0f;   // 更粗
        hoverStyle_.enableGlow = true;
        hoverStyle_.glowColor = 0x00FF0080; // 半透明绿色
        hoverStyle_.glowWidth = 6.0f;       // 更宽的发光

        selectionStyle_.color = 0xFF0000FF; // 红色（更明显）
        selectionStyle_.lineWidth = 3.0f;
        selectionStyle_.enableGlow = true;
        selectionStyle_.glowColor = 0xFF000080; // 半透明红色
        selectionStyle_.glowWidth = 6.0f;

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

    shaderLines_.reset();
    shaderDottedLines_.reset();
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

        // ✅ v0.2: 更新选择状态
        auto it = batches_.find(e->id);
        if (it != batches_.end())
        {
            it->second.selected = e->selected;
            it->second.hovered = e->hovered;
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
        case EntityType::Rectangle:
        {
            uploadRectangle_(e->id, std::get<Rectangle>(e->geom), e->style.rgba, vp);
        }
        break;
        
        case EntityType::GizmoAxis:
        {
            uploadGizmoAxis_(e->id, std::get<GizmoAxis>(e->geom), e->style.rgba, vp);
        }
        break;
        }

        // ✅ 更新选择状态和虚线标志
        it = batches_.find(e->id);
        if (it != batches_.end())
        {
            it->second.selected = e->selected;
            it->second.hovered = e->hovered;   // ⭐ 修复：设置 hover 状态
            it->second.doted = e->dot;         // 设置虚线标志
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
    if (!shaderLines_ || !shader_hover_Lines_ || !shader_hover_Solid_)
    {
        qWarning() << "Shader not initialized";
        return;
    }

    // ============================================
    // 准备通用参数
    // ============================================
    glm::mat4 model(1.0f);
    glm::mat4 mvp = vp.proj * vp.view * model;

    // 清除错误
    while (glGetError() != GL_NO_ERROR)
        ;

    // ============================================
    // 单次遍历，分层绘制
    // ============================================
    for (const auto &kv : batches_)
    {
        const GpuBatch &batch = kv.second;

        // 跳过无效批次
        if (batch.indexCount == 0 || batch.vao == 0)
            continue;

        // 判断几何类型
        bool isLine = (batch.drawMode == GL_LINES ||
                       batch.drawMode == GL_LINE_STRIP ||
                       batch.drawMode == GL_LINE_LOOP);
        bool isSolid = (batch.drawMode == GL_TRIANGLES);

        // ────────────────────────────────────────
        // 1️⃣ 绘制普通状态（未选中且未hover）
        // ────────────────────────────────────────
        if (!batch.selected && !batch.hovered)
        {
            if (isLine)
            {
                // 选择着色器：如果是虚线则用虚线着色器，否则用普通着色器
                Shader *activeShader = batch.doted ? shaderDottedLines_.get() : shaderLines_.get();
                activeShader->use();
                activeShader->setMat4("mvp", mvp);

                float r = ((batch.rgba >> 24) & 0xFF) / 255.0f;
                float g = ((batch.rgba >> 16) & 0xFF) / 255.0f;
                float b = ((batch.rgba >> 8) & 0xFF) / 255.0f;
                float a = ((batch.rgba) & 0xFF) / 255.0f;

                activeShader->setVec4("color", glm::vec4(r, g, b, a));

                // 虚线参数（如果使用虚线着色器）
                if (batch.doted)
                {
                    activeShader->setFloat("dashLength", 0.2f); // 虚线段长度
                    activeShader->setFloat("gapLength", 0.1f);  // 间隙长度
                }

                glBindVertexArray(batch.vao);
                if (batch.ibo)
                    glDrawElements(batch.drawMode, batch.indexCount, GL_UNSIGNED_INT, nullptr);
                else
                    glDrawArrays(batch.drawMode, 0, batch.indexCount);
                glBindVertexArray(0);
            }
            else if (isSolid)
            {
                // 先用shaderLines_顶着，后续根据需求添加指定的着色器
                shaderLines_->use();
                shaderLines_->setMat4("mvp", mvp);
                // shaderSolid_->setMat4("model", model);

                float r = ((batch.rgba >> 24) & 0xFF) / 255.0f;
                float g = ((batch.rgba >> 16) & 0xFF) / 255.0f;
                float b = ((batch.rgba >> 8) & 0xFF) / 255.0f;
                float a = ((batch.rgba) & 0xFF) / 255.0f;

                shaderLines_->setVec4("color", glm::vec4(r, g, b, a));

                glBindVertexArray(batch.vao);
                if (batch.ibo)
                    glDrawElements(batch.drawMode, batch.indexCount, GL_UNSIGNED_INT, nullptr);
                else
                    glDrawArrays(batch.drawMode, 0, batch.indexCount);
                glBindVertexArray(0);
            }
        }

        // ────────────────────────────────────────
        // 2️⃣ 绘制选中状态（高亮）
        // ────────────────────────────────────────
        if (batch.selected)
        {
            float sr = ((selectionColor_ >> 24) & 0xFF) / 255.0f;
            float sg = ((selectionColor_ >> 16) & 0xFF) / 255.0f;
            float sb = ((selectionColor_ >> 8) & 0xFF) / 255.0f;
            float sa = ((selectionColor_) & 0xFF) / 255.0f;

            if (isLine)
            {
                // 使用加粗线条
                shader_hover_Lines_->use();
                shader_hover_Lines_->setMat4("mvp", mvp);
                shader_hover_Lines_->setVec2("viewport", glm::vec2(vp.width, vp.height));
                shader_hover_Lines_->setFloat("thickness", selectionStyle_.lineWidth);
                shader_hover_Lines_->setVec4("color", glm::vec4(sr, sg, sb, sa));

                glBindVertexArray(batch.vao);
                if (batch.ibo)
                    glDrawElements(batch.drawMode, batch.indexCount, GL_UNSIGNED_INT, nullptr);
                else
                    glDrawArrays(batch.drawMode, 0, batch.indexCount);
                glBindVertexArray(0);
            }
            else if (isSolid)
            {
                // 使用发光实心着色器
                shader_hover_Solid_->use();
                shader_hover_Solid_->setMat4("mvp", mvp);
                // shader_hover_Solid_->setMat4("model", model);
                // shader_hover_Solid_->setVec3("lightDir", glm::vec3(0.5f, 0.8f, 0.6f));
                // shader_hover_Solid_->setFloat("glowIntensity", 1.5f); // 选中时更强的发光
                shader_hover_Solid_->setVec4("color", glm::vec4(sr, sg, sb, sa));

                glBindVertexArray(batch.vao);
                if (batch.ibo)
                    glDrawElements(batch.drawMode, batch.indexCount, GL_UNSIGNED_INT, nullptr);
                else
                    glDrawArrays(batch.drawMode, 0, batch.indexCount);
                glBindVertexArray(0);
            }
        }

        // ────────────────────────────────────────
        // 3️⃣ 绘制Hover状态（双层效果）
        // ────────────────────────────────────────
        if (batch.hovered && !batch.selected)
        {
            float gr = ((hoverStyle_.glowColor >> 24) & 0xFF) / 255.0f;
            float gg = ((hoverStyle_.glowColor >> 16) & 0xFF) / 255.0f;
            float gb = ((hoverStyle_.glowColor >> 8) & 0xFF) / 255.0f;
            float ga = ((hoverStyle_.glowColor) & 0xFF) / 255.0f;

            float hr = ((hoverStyle_.color >> 24) & 0xFF) / 255.0f;
            float hg = ((hoverStyle_.color >> 16) & 0xFF) / 255.0f;
            float hb = ((hoverStyle_.color >> 8) & 0xFF) / 255.0f;
            float ha = ((hoverStyle_.color) & 0xFF) / 255.0f;

            if (isLine)
            {
                shader_hover_Lines_->use();
                shader_hover_Lines_->setMat4("mvp", mvp);
                shader_hover_Lines_->setVec2("viewport", glm::vec2(vp.width, vp.height));

                // 第一层：外发光（更粗、半透明）
                shader_hover_Lines_->setFloat("thickness", hoverStyle_.lineWidth * 2.5f);
                shader_hover_Lines_->setVec4("color", glm::vec4(gr, gg, gb, ga));

                glBindVertexArray(batch.vao);
                if (batch.ibo)
                    glDrawElements(batch.drawMode, batch.indexCount, GL_UNSIGNED_INT, nullptr);
                else
                    glDrawArrays(batch.drawMode, 0, batch.indexCount);
                glBindVertexArray(0);

                // 第二层：实体线条（正常粗细、不透明）
                shader_hover_Lines_->setFloat("thickness", hoverStyle_.lineWidth);
                shader_hover_Lines_->setVec4("color", glm::vec4(hr, hg, hb, ha));

                glBindVertexArray(batch.vao);
                if (batch.ibo)
                    glDrawElements(batch.drawMode, batch.indexCount, GL_UNSIGNED_INT, nullptr);
                else
                    glDrawArrays(batch.drawMode, 0, batch.indexCount);
                glBindVertexArray(0);
            }
            else if (isSolid)
            {
                // ✅ 立方体hover效果（超简化版本）
                shader_hover_Solid_->use();
                shader_hover_Solid_->setMat4("mvp", mvp);

                // 第一层：外发光（半透明绿色）
                shader_hover_Solid_->setVec4("color", glm::vec4(gr, gg, gb, ga));

                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                glBindVertexArray(batch.vao);
                if (batch.ibo)
                    glDrawElements(batch.drawMode, batch.indexCount, GL_UNSIGNED_INT, nullptr);
                else
                    glDrawArrays(batch.drawMode, 0, batch.indexCount);
                glBindVertexArray(0);

                // 第二层：实体（不透明绿色）
                shader_hover_Solid_->setVec4("color", glm::vec4(hr, hg, hb, ha));

                glBindVertexArray(batch.vao);
                if (batch.ibo)
                    glDrawElements(batch.drawMode, batch.indexCount, GL_UNSIGNED_INT, nullptr);
                else
                    glDrawArrays(batch.drawMode, 0, batch.indexCount);
                glBindVertexArray(0);

                glDisable(GL_BLEND);
            }
        }
    }

    // 检查错误
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
    {
        qWarning() << "OpenGL error in draw():" << err;
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

void Renderer::uploadRectangle_(EntityId id, const Rectangle &L, std::uint32_t rgba, const ViewportState& vp)
{
    GpuBatch b{};

    // ✅ p0和p1已经通过射线投影到工作平面上
    // 工作平面垂直于摄像机视线方向（面向摄像机）
    glm::vec3 p0 = L.p0;
    glm::vec3 p1 = L.p1;
    
    // 1. 从视图矩阵提取摄像机前向量（工作平面的法线）
    glm::mat4 invView = glm::inverse(vp.view);
    glm::vec3 cameraFront = -glm::normalize(glm::vec3(invView[2])); // 视图空间的-Z轴即前向量
    
    // 2. 计算工作平面的两个正交轴
    // 使用屏幕对齐的方式：右向量和上向量
    glm::vec3 planeRight = glm::normalize(glm::vec3(invView[0]));  // 视图空间的X轴（右）
    glm::vec3 planeUp = glm::normalize(glm::vec3(invView[1]));     // 视图空间的Y轴（上）
    
    // 3. 将对角线向量(p0->p1)分解到工作平面的两个轴上
    glm::vec3 diagonal = p1 - p0;
    float rightComponent = glm::dot(diagonal, planeRight);  // 在右轴上的分量
    float upComponent = glm::dot(diagonal, planeUp);        // 在上轴上的分量
    
    // 4. 在工作平面上构建矩形的4个顶点
    // 这样构建的矩形一定在工作平面上，且面向摄像机
    glm::vec3 v0 = p0;                                      // 左下角
    glm::vec3 v1 = p0 + planeRight * rightComponent;       // 右下角（沿右轴）
    glm::vec3 v2 = p1;                                      // 右上角（对角点）
    glm::vec3 v3 = p0 + planeUp * upComponent;             // 左上角（沿上轴）

    // ✅ 使用3D距离计算各边长度
    float edge0 = glm::distance(v0, v1); // 底边
    float edge1 = glm::distance(v1, v2); // 右边
    float edge2 = glm::distance(v2, v3); // 顶边
    float edge3 = glm::distance(v3, v0); // 左边
    float perimeter = edge0 + edge1 + edge2 + edge3; // 完整周长

    // 使用扩展的顶点格式，包含位置和沿线距离
    // 关键：v0出现两次，第二次距离=完整周长，确保第四条边距离递增
    std::vector<PosDistVertex> vb = {
        {v0, 0.0f},                      // v0: 起点，距离=0
        {v1, edge0},                     // v1: 底边终点
        {v2, edge0 + edge1},             // v2: 右边终点
        {v3, edge0 + edge1 + edge2},     // v3: 顶边终点
        {v0, perimeter}                  // v4: 回到起点，距离=完整周长
    };

    // ✅ 索引：4条独立的线段
    // 注意：v0在顶点数组中出现两次（索引0和4），第四条边使用索引4
    std::vector<GLuint> indices = {
        0, 1, // 上边: v0→v1, dist从0到width
        1, 2, // 右边: v1→v2, dist从width到width+height
        2, 3, // 下边: v2→v3, dist从width+height到2*width+height
        3, 4  // 左边: v3→v4(=v0), dist从2*width+height到perimeter ✓
    };

    glGenBuffers(1, &b.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, b.vbo);
    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(vb.size() * sizeof(PosDistVertex)), vb.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &b.ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, b.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(indices.size() * sizeof(GLuint)), indices.data(), GL_STATIC_DRAW);

    // ✅ 创建VAO并绑定顶点属性
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, b.vbo);

    // 属性0: 位置（vec3）
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PosDistVertex), (void *)0);
    glEnableVertexAttribArray(0);

    // 属性1: 距离（float）
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(PosDistVertex),
                          (void *)offsetof(PosDistVertex, dist));
    glEnableVertexAttribArray(1);

    if (b.ibo)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, b.ibo);
    }
    glBindVertexArray(0);

    b.vao = vao;
    b.indexCount = GLsizei(indices.size());
    b.rgba = rgba;
    b.drawMode = GL_LINES; // 使用GL_LINES绘制4条独立的线段
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

void Renderer::uploadGizmoAxis_(EntityId id, const GizmoAxis& G, std::uint32_t rgba, const ViewportState& vp)
{
    GpuBatch b{};
    
    // 计算轴的端点
    glm::vec3 axisEnd = G.origin + G.direction * G.length;
    
    // 计算箭头参数
    float arrowSize = G.length * 0.15f;
    glm::vec3 arrowBase = axisEnd - G.direction * arrowSize;
    
    // 计算垂直于轴的两个方向
    glm::vec3 perpDir1, perpDir2;
    if (std::abs(G.direction.x) < 0.9f) {
        perpDir1 = glm::normalize(glm::cross(G.direction, glm::vec3(1, 0, 0)));
    } else {
        perpDir1 = glm::normalize(glm::cross(G.direction, glm::vec3(0, 1, 0)));
    }
    perpDir2 = glm::cross(G.direction, perpDir1);
    
    float arrowWidth = arrowSize * 0.3f;
    
    // 构建顶点：轴线 + 箭头的4条边
    std::vector<PosVertex> vb;
    vb.reserve(10);
    
    // 轴线（2个顶点）
    vb.push_back({G.origin});
    vb.push_back({axisEnd});
    
    // 箭头的4条边（8个顶点）
    vb.push_back({arrowBase + perpDir1 * arrowWidth});
    vb.push_back({axisEnd});
    vb.push_back({arrowBase - perpDir1 * arrowWidth});
    vb.push_back({axisEnd});
    vb.push_back({arrowBase + perpDir2 * arrowWidth});
    vb.push_back({axisEnd});
    vb.push_back({arrowBase - perpDir2 * arrowWidth});
    vb.push_back({axisEnd});
    
    // 上传到 GPU
    glGenBuffers(1, &b.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, b.vbo);
    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(vb.size() * sizeof(PosVertex)), vb.data(), GL_STATIC_DRAW);
    
    b.vao = makeVao(b.vbo, 0);
    b.indexCount = GLsizei(vb.size());
    b.rgba = rgba;
    b.drawMode = GL_LINES;
    batches_[id] = b;
}