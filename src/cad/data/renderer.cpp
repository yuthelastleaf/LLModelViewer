#include "renderer.h"
#include <cmath>

// ========== 最小着色器（位置+统一颜色） ==========
static const char* kVS = R"(#version 330 core
layout(location=0) in vec3 aPos;
uniform mat4 uMVP;
void main(){ gl_Position = uMVP * vec4(aPos,1.0); }
)";
static const char* kFS = R"(#version 330 core
uniform vec4 uColor;
out vec4 FragColor;
void main(){ FragColor = uColor; }
)";

bool Renderer::initialize() {
    // 初始化 OpenGL 函数
    if (!initializeOpenGLFunctions()) {
        return false;
    }
    
    if (!progLines_.addShaderFromSourceCode(QOpenGLShader::Vertex, kVS)) return false;
    if (!progLines_.addShaderFromSourceCode(QOpenGLShader::Fragment, kFS)) return false;
    if (!progLines_.link()) return false;
    uMVP_   = progLines_.uniformLocation("uMVP");
    uColor_ = progLines_.uniformLocation("uColor");
    return true;
}

void Renderer::shutdown() {
    for (auto& kv : batches_) freeBatch_(kv.second);
    batches_.clear();
    progLines_.removeAllShaders();
}

GLuint Renderer::makeVao(GLuint vbo, GLuint ibo) {
    GLuint vao=0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PosVertex), (void*)0);
    if (ibo) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBindVertexArray(0);
    return vao;
}

void Renderer::freeBatch_(GpuBatch& b) {
    if (b.ibo) glDeleteBuffers(1, &b.ibo), b.ibo = 0;
    if (b.vbo) glDeleteBuffers(1, &b.vbo), b.vbo = 0;
    if (b.vao) glDeleteVertexArrays(1, &b.vao), b.vao = 0;
    b.indexCount = 0;
}

// 修正：RGBA 格式 0xRRGGBBAA
static glm::vec4 rgbaToVec4(std::uint32_t rgba) {
    float r = ((rgba >> 24) & 0xFF) / 255.0f;
    float g = ((rgba >> 16) & 0xFF) / 255.0f;
    float b = ((rgba >>  8) & 0xFF) / 255.0f;
    float a = ((rgba >>  0) & 0xFF) / 255.0f;
    return {r,g,b,a};
}

void ViewportState::updateWorldPerPixel() {
    if (width <= 0 || height <= 0) {
        worldPerPixel = 1.0f;
        return;
    }
    // 简化计算：取视图矩阵的缩放因子倒数
    glm::vec4 origin = view * glm::vec4(0,0,0,1);
    glm::vec4 offset = view * glm::vec4(1,0,0,1);
    float viewScale = glm::length(glm::vec3(offset - origin));
    
    // 透视投影的近似值
    float ndcWidth = 2.0f;  // NDC 空间宽度
    worldPerPixel = ndcWidth / (viewScale * float(width));
}

void Renderer::syncFromDocument(const Document& doc, const ViewportState& vp, bool forceRebuild) {
    // 检查是否需要因缩放级别变化而重新细分圆弧
    bool needRetessellate = forceRebuild || 
        (std::abs(vp.worldPerPixel - lastWorldPerPixel_) / vp.worldPerPixel > 0.5f);
    
    if (needRetessellate) {
        lastWorldPerPixel_ = vp.worldPerPixel;
    }
    
    if (forceRebuild) {
        // 全量重建
        for (auto& kv : batches_) freeBatch_(kv.second);
        batches_.clear();
    }

    for (auto* e : doc.all()) {
        if (!e->visible) {
            // 隐藏的实体：移除批次
            if (batches_.count(e->id)) {
                freeBatch_(batches_[e->id]);
                batches_.erase(e->id);
            }
            continue;
        }
        
        // 只处理脏实体或需要重新细分的圆弧
        bool needUpdate = e->dirty;
        if (!needUpdate && needRetessellate) {
            needUpdate = (e->type == EntityType::Circle || e->type == EntityType::Arc);
        }
        
        if (!needUpdate && batches_.count(e->id)) {
            continue;  // 已有批次且无需更新
        }
        
        // 删除旧批次
        if (batches_.count(e->id)) {
            freeBatch_(batches_[e->id]);
            batches_.erase(e->id);
        }
        
        // 上传新批次
        switch (e->type) {
            case EntityType::Line: {
                uploadLine_(e->id, std::get<Line>(e->geom), e->style.rgba);
            } break;
            case EntityType::Polyline: {
                uploadPolyline_(e->id, std::get<Polyline>(e->geom), e->style.rgba);
            } break;
            case EntityType::Circle: {
                uploadCircle_(e->id, std::get<Circle>(e->geom), e->style.rgba, vp);
            } break;
            case EntityType::Arc: {
                uploadArc_(e->id, std::get<Arc>(e->geom), e->style.rgba, vp);
            } break;
        }
    }
}

void Renderer::removeBatch(EntityId id) {
    auto it = batches_.find(id);
    if (it != batches_.end()) {
        freeBatch_(it->second);
        batches_.erase(it);
    }
}

void Renderer::draw(const ViewportState& vp) {
    progLines_.bind();
    glm::mat4 mvp = vp.proj * vp.view; // v0.1 不用 model
    progLines_.setUniformValue(uMVP_, QMatrix4x4(&mvp[0][0]).transposed()); // 注意 Qt 行列主序
    glLineWidth(1.0f); // Core Profile 下通常只支持 1

    for (auto& kv : batches_) {
        auto& b = kv.second;
        glm::vec4 c = rgbaToVec4(b.rgba);
        progLines_.setUniformValue(uColor_, QVector4D(c.r, c.g, c.b, c.a));
        glBindVertexArray(b.vao);
        
        if (b.drawMode == GL_LINES) {
            glDrawArrays(GL_LINES, 0, b.indexCount);
        } else { // GL_LINE_STRIP
            glDrawArrays(GL_LINE_STRIP, 0, b.indexCount);
        }
    }
    glBindVertexArray(0);
    progLines_.release();
}

void Renderer::drawLineStrip(const std::vector<glm::vec3>& pts, std::uint32_t rgba, const ViewportState& vp) {
    if (pts.size() < 2) return;

    // 临时上传（小批用于网格/轴）
    GLuint vbo=0, vao=0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    std::vector<PosVertex> vb(pts.size());
    for (size_t i=0;i<pts.size();++i) vb[i].pos = pts[i];
    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(vb.size()*sizeof(PosVertex)), vb.data(), GL_STREAM_DRAW);
    vao = makeVao(vbo, 0);

    progLines_.bind();
    glm::mat4 mvp = vp.proj * vp.view;
    progLines_.setUniformValue(uMVP_, QMatrix4x4(&mvp[0][0]).transposed());
    glm::vec4 c = rgbaToVec4(rgba);
    progLines_.setUniformValue(uColor_, QVector4D(c.r, c.g, c.b, c.a));

    glBindVertexArray(vao);
    glDrawArrays(GL_LINE_STRIP, 0, GLsizei(pts.size()));
    glBindVertexArray(0);
    progLines_.release();

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

void Renderer::drawLineSegments(const std::vector<glm::vec3>& ptsPairs, std::uint32_t rgba, const ViewportState& vp) {
    if (ptsPairs.size() < 2) return;
    // 期望偶数个点，成对构成线段
    GLuint vbo=0, vao=0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    std::vector<PosVertex> vb(ptsPairs.size());
    for (size_t i=0;i<ptsPairs.size();++i) vb[i].pos = ptsPairs[i];
    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(vb.size()*sizeof(PosVertex)), vb.data(), GL_STREAM_DRAW);
    vao = makeVao(vbo, 0);

    progLines_.bind();
    glm::mat4 mvp = vp.proj * vp.view;
    progLines_.setUniformValue(uMVP_, QMatrix4x4(&mvp[0][0]).transposed());
    glm::vec4 c = rgbaToVec4(rgba);
    progLines_.setUniformValue(uColor_, QVector4D(c.r, c.g, c.b, c.a));

    glBindVertexArray(vao);
    glDrawArrays(GL_LINES, 0, GLsizei(ptsPairs.size()));
    glBindVertexArray(0);
    progLines_.release();

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

// ========== 上传实体 ==========
void Renderer::uploadLine_(EntityId id, const Line& L, std::uint32_t rgba) {
    GpuBatch b{};
    std::vector<PosVertex> vb = {{L.p0}, {L.p1}};
    glGenBuffers(1, &b.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, b.vbo);
    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(vb.size()*sizeof(PosVertex)), vb.data(), GL_STATIC_DRAW);
    b.vao = makeVao(b.vbo, 0);
    b.indexCount = GLsizei(vb.size());
    b.rgba = rgba;
    b.drawMode = GL_LINES;
    batches_[id] = b;
}

void Renderer::uploadPolyline_(EntityId id, const Polyline& P, std::uint32_t rgba) {
    if (P.pts.size() < 2) return;
    GpuBatch b{};
    std::vector<PosVertex> vb(P.pts.size() + (P.closed ? 1 : 0));
    for (size_t i=0;i<P.pts.size();++i) vb[i].pos = P.pts[i];
    if (P.closed) vb.back().pos = P.pts.front();

    glGenBuffers(1, &b.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, b.vbo);
    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(vb.size()*sizeof(PosVertex)), vb.data(), GL_STATIC_DRAW);
    b.vao = makeVao(b.vbo, 0);
    b.indexCount = GLsizei(vb.size());
    b.rgba = rgba;
    b.drawMode = GL_LINE_STRIP;
    batches_[id] = b;
}

void Renderer::uploadCircle_(EntityId id, const Circle& C, std::uint32_t rgba, const ViewportState& vp) {
    float worldEps = vp.worldPerPixel * 0.5f;
    auto pts = tessellateCircle(C, worldEps);
    Polyline P{pts, true};
    uploadPolyline_(id, P, rgba);
}

void Renderer::uploadArc_(EntityId id, const Arc& A, std::uint32_t rgba, const ViewportState& vp) {
    float worldEps = vp.worldPerPixel * 0.5f;
    auto pts = tessellateArc(A, worldEps);
    Polyline P{pts, false};
    uploadPolyline_(id, P, rgba);
}

// ========== 细分：保证弧边弦高误差约 <= worldEps ==========
static int segsForRadius(float r, float worldEps, float spanRadians) {
    // chord error e ≈ r * (1 - cos(theta/2)) <= worldEps
    // theta_max ≈ 2 * acos(1 - e/r)
    if (r < 1e-6f) return 8;
    float thetaMax = 2.0f * std::acos(std::max(0.0f, 1.0f - worldEps / r));
    if (thetaMax <= 0.0f) thetaMax = 0.1f; // fallback
    int n = int(std::ceil(spanRadians / thetaMax));
    return std::max(8, std::min(n, 360)); // 限制在 [8, 360]
}

std::vector<glm::vec3> Renderer::tessellateCircle(const Circle& C, float worldEps) {
    int n = segsForRadius(C.r, worldEps, 2.0f * float(M_PI));
    std::vector<glm::vec3> pts; pts.reserve(n);
    for (int i=0;i<n;i++) {
        float t = (float(i) / float(n)) * 2.0f * float(M_PI);
        pts.push_back({ C.c.x + C.r*std::cos(t), C.c.y + C.r*std::sin(t), C.c.z });
    }
    return pts;
}

std::vector<glm::vec3> Renderer::tessellateArc(const Arc& A, float worldEps) {
    float span = A.a1 - A.a0;
    // 归一化到 [0, 2pi]
    while (span < 0) span += 2.0f*float(M_PI);
    while (span > 2.0f*float(M_PI)) span -= 2.0f*float(M_PI);

    int n = segsForRadius(A.r, worldEps, span);
    n = std::max(2, n);
    std::vector<glm::vec3> pts; pts.reserve(n+1);
    for (int i=0;i<=n;i++) {
        float t = A.a0 + span * (float(i)/float(n));
        pts.push_back({ A.c.x + A.r*std::cos(t), A.c.y + A.r*std::sin(t), A.c.z });
    }
    return pts;
}