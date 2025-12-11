#include "DxfHandler.h"
#include <QDebug>
#include <cmath>

// 静态成员初始化
QString DxfHandler::lastError_;

// ============================================================================
// 主要接口
// ============================================================================

bool DxfHandler::importFromFile(const QString& filePath, Document* document) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        lastError_ = QString("无法打开文件: %1").arg(filePath);
        return false;
    }

    QTextStream stream(&file);
    DxfPair pair;
    std::map<QString, DxfLayer> layers;
    QString currentSection;

    while (readPair(stream, pair)) {
        if (pair.code == 0 && pair.value.toUpper() == "SECTION") {
            if (readPair(stream, pair) && pair.code == 2) {
                currentSection = pair.value.toUpper();
            }
        }
        else if (pair.code == 0 && pair.value.toUpper() == "ENDSEC") {
            currentSection.clear();
        }
        else if (currentSection == "TABLES") {
            // 解析图层等表格（简化版本，暂时跳过）
        }
        else if (currentSection == "ENTITIES") {
            if (pair.code == 0) {
                QString entityType = pair.value.toUpper();
                if (entityType == "LINE") {
                    parseLine(stream, document, layers);
                }
                else if (entityType == "CIRCLE") {
                    parseCircle(stream, document, layers);
                }
                else if (entityType == "ARC") {
                    parseArc(stream, document, layers);
                }
                else if (entityType == "LWPOLYLINE") {
                    parseLwPolyline(stream, document, layers);
                }
                else if (entityType == "POLYLINE") {
                    parsePolyline(stream, document, layers);
                }
                else if (entityType == "SPLINE") {
                    parseSpline(stream, document, layers);
                }
            }
        }
    }

    file.close();
    return true;
}

bool DxfHandler::exportToFile(const QString& filePath, const Document* document) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        lastError_ = QString("无法创建文件: %1").arg(filePath);
        return false;
    }

    QTextStream stream(&file);
    stream.setRealNumberPrecision(6);
    stream.setRealNumberNotation(QTextStream::FixedNotation);

    writeHeader(stream);
    writeTables(stream, document);
    writeEntities(stream, document);
    writeEof(stream);

    file.close();
    return true;
}

// ============================================================================
// DXF 读取
// ============================================================================

bool DxfHandler::readPair(QTextStream& stream, DxfPair& pair) {
    if (stream.atEnd()) return false;
    
    QString codeLine = stream.readLine().trimmed();
    if (stream.atEnd()) return false;
    QString valueLine = stream.readLine().trimmed();

    bool ok;
    pair.code = codeLine.toInt(&ok);
    if (!ok) return false;
    
    pair.value = valueLine;
    return true;
}

void DxfHandler::skipToSection(QTextStream& stream, const QString& sectionName) {
    DxfPair pair;
    while (readPair(stream, pair)) {
        if (pair.code == 0 && pair.value.toUpper() == "SECTION") {
            if (readPair(stream, pair) && pair.code == 2) {
                if (pair.value.toUpper() == sectionName.toUpper()) {
                    return;
                }
            }
        }
    }
}

void DxfHandler::skipToEndSec(QTextStream& stream) {
    DxfPair pair;
    while (readPair(stream, pair)) {
        if (pair.code == 0 && pair.value.toUpper() == "ENDSEC") {
            return;
        }
    }
}

// ============================================================================
// 实体解析 - 步骤 3: LINE
// ============================================================================

void DxfHandler::parseLine(QTextStream& stream, Document* document,
                           const std::map<QString, DxfLayer>& layers) {
    glm::vec3 p0(0.0f), p1(0.0f);
    int color = 7;
    QString layerName = "0";
    
    DxfPair pair;
    qint64 lastPos = stream.pos();
    
    while (readPair(stream, pair)) {
        switch (pair.code) {
            case 8:  layerName = pair.value; break;
            case 62: color = pair.value.toInt(); break;
            case 10: p0.x = pair.value.toFloat(); break;
            case 20: p0.y = pair.value.toFloat(); break;
            case 30: p0.z = pair.value.toFloat(); break;
            case 11: p1.x = pair.value.toFloat(); break;
            case 21: p1.y = pair.value.toFloat(); break;
            case 31: p1.z = pair.value.toFloat(); break;
            case 0:
                // 遇到新实体，回退并退出
                stream.seek(lastPos);
                goto done;
            default:
                break;
        }
        lastPos = stream.pos();
    }
    
done:
    Style style;
    style.rgba = aciToRgba(color);
    document->addLine(p0, p1, style);
}

// ============================================================================
// 实体解析 - 步骤 4: CIRCLE / ARC
// ============================================================================

void DxfHandler::parseCircle(QTextStream& stream, Document* document,
                             const std::map<QString, DxfLayer>& layers) {
    glm::vec3 center(0.0f);
    float radius = 0.0f;
    int color = 7;
    
    DxfPair pair;
    qint64 lastPos = stream.pos();
    
    while (readPair(stream, pair)) {
        switch (pair.code) {
            case 8:  /* layerName */ break;
            case 62: color = pair.value.toInt(); break;
            case 10: center.x = pair.value.toFloat(); break;
            case 20: center.y = pair.value.toFloat(); break;
            case 30: center.z = pair.value.toFloat(); break;
            case 40: radius = pair.value.toFloat(); break;
            case 0:
                stream.seek(lastPos);
                goto done_circle;
            default:
                break;
        }
        lastPos = stream.pos();
    }
    
done_circle:
    if (radius > 0.0f) {
        Style style;
        style.rgba = aciToRgba(color);
        document->addCircle(center, radius, style);
    }
}

void DxfHandler::parseArc(QTextStream& stream, Document* document,
                          const std::map<QString, DxfLayer>& layers) {
    glm::vec3 center(0.0f);
    float radius = 0.0f;
    float startAngle = 0.0f;  // 度
    float endAngle = 360.0f;  // 度
    int color = 7;
    
    DxfPair pair;
    qint64 lastPos = stream.pos();
    
    while (readPair(stream, pair)) {
        switch (pair.code) {
            case 8:  /* layerName */ break;
            case 62: color = pair.value.toInt(); break;
            case 10: center.x = pair.value.toFloat(); break;
            case 20: center.y = pair.value.toFloat(); break;
            case 30: center.z = pair.value.toFloat(); break;
            case 40: radius = pair.value.toFloat(); break;
            case 50: startAngle = pair.value.toFloat(); break;
            case 51: endAngle = pair.value.toFloat(); break;
            case 0:
                stream.seek(lastPos);
                goto done_arc;
            default:
                break;
        }
        lastPos = stream.pos();
    }
    
done_arc:
    if (radius > 0.0f) {
        Style style;
        style.rgba = aciToRgba(color);
        // DXF 使用度，转换为弧度
        float a0 = startAngle * 3.14159265f / 180.0f;
        float a1 = endAngle * 3.14159265f / 180.0f;
        document->addArc(center, radius, a0, a1, style);
    }
}

// ============================================================================
// 实体解析 - 步骤 5: POLYLINE
// ============================================================================

void DxfHandler::parseLwPolyline(QTextStream& stream, Document* document,
                                  const std::map<QString, DxfLayer>& layers) {
    std::vector<glm::vec3> points;
    glm::vec3 currentPt(0.0f);
    bool closed = false;
    int color = 7;
    int vertexCount = 0;
    bool hasX = false;
    
    DxfPair pair;
    qint64 lastPos = stream.pos();
    
    while (readPair(stream, pair)) {
        switch (pair.code) {
            case 8:  /* layerName */ break;
            case 62: color = pair.value.toInt(); break;
            case 70: 
                // 标志位：1 = 闭合
                closed = (pair.value.toInt() & 1) != 0;
                break;
            case 90: 
                vertexCount = pair.value.toInt();
                break;
            case 10:
                // 新顶点开始，保存前一个点
                if (hasX) {
                    points.push_back(currentPt);
                }
                currentPt = glm::vec3(0.0f);
                currentPt.x = pair.value.toFloat();
                hasX = true;
                break;
            case 20:
                currentPt.y = pair.value.toFloat();
                break;
            case 30:
                currentPt.z = pair.value.toFloat();
                break;
            case 0:
                stream.seek(lastPos);
                goto done_lwpoly;
            default:
                break;
        }
        lastPos = stream.pos();
    }
    
done_lwpoly:
    // 保存最后一个点
    if (hasX) {
        points.push_back(currentPt);
    }
    
    if (points.size() >= 2) {
        Style style;
        style.rgba = aciToRgba(color);
        document->addPolyline(points, closed, style);
    }
}

void DxfHandler::parsePolyline(QTextStream& stream, Document* document,
                                const std::map<QString, DxfLayer>& layers) {
    // 旧版 POLYLINE 格式：实体后跟 VERTEX 实体，以 SEQEND 结束
    std::vector<glm::vec3> points;
    bool closed = false;
    int color = 7;
    
    DxfPair pair;
    qint64 lastPos = stream.pos();
    
    // 读取 POLYLINE 头部属性
    while (readPair(stream, pair)) {
        if (pair.code == 0) {
            stream.seek(lastPos);
            break;
        }
        if (pair.code == 62) color = pair.value.toInt();
        if (pair.code == 70) closed = (pair.value.toInt() & 1) != 0;
        lastPos = stream.pos();
    }
    
    // 读取 VERTEX 实体
    while (readPair(stream, pair)) {
        if (pair.code == 0) {
            QString entityType = pair.value.toUpper();
            if (entityType == "VERTEX") {
                glm::vec3 pt(0.0f);
                lastPos = stream.pos();
                while (readPair(stream, pair)) {
                    if (pair.code == 0) {
                        stream.seek(lastPos);
                        break;
                    }
                    if (pair.code == 10) pt.x = pair.value.toFloat();
                    if (pair.code == 20) pt.y = pair.value.toFloat();
                    if (pair.code == 30) pt.z = pair.value.toFloat();
                    lastPos = stream.pos();
                }
                points.push_back(pt);
            }
            else if (entityType == "SEQEND") {
                // 跳过 SEQEND 属性
                lastPos = stream.pos();
                while (readPair(stream, pair)) {
                    if (pair.code == 0) {
                        stream.seek(lastPos);
                        break;
                    }
                    lastPos = stream.pos();
                }
                break;
            }
            else {
                // 其他实体，回退
                stream.seek(lastPos);
                break;
            }
        }
        lastPos = stream.pos();
    }
    
    if (points.size() >= 2) {
        Style style;
        style.rgba = aciToRgba(color);
        document->addPolyline(points, closed, style);
    }
}

// ============================================================================
// 实体解析 - SPLINE（样条曲线）
// ============================================================================

// 简单的 De Casteljau 算法计算贝塞尔曲线上的点
static glm::vec3 evaluateBezier(const std::vector<glm::vec3>& controlPoints, float t) {
    std::vector<glm::vec3> points = controlPoints;
    int n = static_cast<int>(points.size());
    
    for (int r = 1; r < n; ++r) {
        for (int i = 0; i < n - r; ++i) {
            points[i] = (1.0f - t) * points[i] + t * points[i + 1];
        }
    }
    return points[0];
}

// B-Spline 基函数（Cox-de Boor 递归公式）
static float bsplineBasis(int i, int k, float t, const std::vector<float>& knots) {
    if (k == 1) {
        if (t >= knots[i] && t < knots[i + 1]) return 1.0f;
        return 0.0f;
    }
    
    float d1 = knots[i + k - 1] - knots[i];
    float d2 = knots[i + k] - knots[i + 1];
    
    float c1 = 0.0f, c2 = 0.0f;
    
    if (d1 > 1e-6f) {
        c1 = (t - knots[i]) / d1 * bsplineBasis(i, k - 1, t, knots);
    }
    if (d2 > 1e-6f) {
        c2 = (knots[i + k] - t) / d2 * bsplineBasis(i + 1, k - 1, t, knots);
    }
    
    return c1 + c2;
}

// 计算 B-Spline 曲线上的点
static glm::vec3 evaluateBSpline(const std::vector<glm::vec3>& controlPoints, 
                                  const std::vector<float>& knots,
                                  int degree, float t) {
    glm::vec3 result(0.0f);
    int n = static_cast<int>(controlPoints.size());
    
    for (int i = 0; i < n; ++i) {
        float basis = bsplineBasis(i, degree + 1, t, knots);
        result += basis * controlPoints[i];
    }
    
    return result;
}

void DxfHandler::parseSpline(QTextStream& stream, Document* document,
                              const std::map<QString, DxfLayer>& layers) {
    std::vector<glm::vec3> controlPoints;
    std::vector<float> knots;
    glm::vec3 currentPt(0.0f);
    bool closed = false;
    int color = 7;
    int degree = 3;      // 默认三次样条
    int numKnots = 0;
    int numControlPoints = 0;
    int knotIndex = 0;
    int ctrlPtIndex = 0;
    bool readingControlPoint = false;
    
    DxfPair pair;
    qint64 lastPos = stream.pos();
    
    while (readPair(stream, pair)) {
        switch (pair.code) {
            case 8:  /* layerName */ break;
            case 62: color = pair.value.toInt(); break;
            case 70: 
                // 标志位：1 = 闭合
                closed = (pair.value.toInt() & 1) != 0;
                break;
            case 71:
                degree = pair.value.toInt();
                break;
            case 72:
                numKnots = pair.value.toInt();
                knots.reserve(numKnots);
                break;
            case 73:
                numControlPoints = pair.value.toInt();
                controlPoints.reserve(numControlPoints);
                break;
            case 40:
                // 节点值
                knots.push_back(pair.value.toFloat());
                break;
            case 10:
                // 控制点 X
                currentPt.x = pair.value.toFloat();
                readingControlPoint = true;
                break;
            case 20:
                // 控制点 Y
                currentPt.y = pair.value.toFloat();
                break;
            case 30:
                // 控制点 Z，完成一个控制点
                currentPt.z = pair.value.toFloat();
                if (readingControlPoint) {
                    controlPoints.push_back(currentPt);
                    currentPt = glm::vec3(0.0f);
                    readingControlPoint = false;
                }
                break;
            case 0:
                stream.seek(lastPos);
                goto done_spline;
            default:
                break;
        }
        lastPos = stream.pos();
    }
    
done_spline:
    // 如果有未保存的控制点（没有Z坐标的情况）
    if (readingControlPoint) {
        controlPoints.push_back(currentPt);
    }
    
    if (controlPoints.size() < 2) {
        return;
    }
    
    Style style;
    style.rgba = aciToRgba(color);
    
    // 将样条曲线离散化为多段线
    std::vector<glm::vec3> polylinePoints;
    int segments = std::max(20, static_cast<int>(controlPoints.size()) * 10);
    
    if (!knots.empty() && knots.size() >= controlPoints.size() + degree + 1) {
        // 使用 B-Spline 评估
        float tMin = knots[degree];
        float tMax = knots[knots.size() - degree - 1];
        
        for (int i = 0; i <= segments; ++i) {
            float t = tMin + (tMax - tMin) * i / segments;
            // 处理最后一个点，避免精度问题
            if (i == segments) t = tMax - 1e-6f;
            glm::vec3 pt = evaluateBSpline(controlPoints, knots, degree, t);
            polylinePoints.push_back(pt);
        }
    } else {
        // 简单贝塞尔曲线近似
        for (int i = 0; i <= segments; ++i) {
            float t = static_cast<float>(i) / segments;
            glm::vec3 pt = evaluateBezier(controlPoints, t);
            polylinePoints.push_back(pt);
        }
    }
    
    if (polylinePoints.size() >= 2) {
        document->addPolyline(polylinePoints, closed, style);
    }
}

// ============================================================================
// 图层解析（简化版）
// ============================================================================

std::map<QString, DxfHandler::DxfLayer> DxfHandler::parseTablesSection(QTextStream& stream) {
    std::map<QString, DxfLayer> layers;
    // 简化：返回空表，使用默认图层
    return layers;
}

// ============================================================================
// DXF 写入 - 步骤 6 （占位）
// ============================================================================

void DxfHandler::writeHeader(QTextStream& stream) {
    stream << "0\nSECTION\n";
    stream << "2\nHEADER\n";
    stream << "9\n$ACADVER\n";
    stream << "1\nAC1015\n";  // AutoCAD 2000
    stream << "0\nENDSEC\n";
}

void DxfHandler::writeTables(QTextStream& stream, const Document* document) {
    // 最小化 TABLES 段
    stream << "0\nSECTION\n";
    stream << "2\nTABLES\n";
    stream << "0\nENDSEC\n";
}

void DxfHandler::writeEntities(QTextStream& stream, const Document* document) {
    stream << "0\nSECTION\n";
    stream << "2\nENTITIES\n";
    
    auto entities = document->all();
    for (const auto* entity : entities) {
        if (!entity->visible || entity->isGizmo) continue;
        
        switch (entity->type) {
            case EntityType::Line:
                writeLine(stream, std::get<Line>(entity->geom), entity->style);
                break;
            case EntityType::Circle:
                writeCircle(stream, std::get<Circle>(entity->geom), entity->style);
                break;
            case EntityType::Arc:
                writeArc(stream, std::get<Arc>(entity->geom), entity->style);
                break;
            case EntityType::Polyline:
                writePolyline(stream, std::get<Polyline>(entity->geom), entity->style);
                break;
            case EntityType::Rectangle:
                writeRectangle(stream, std::get<Rectangle>(entity->geom), entity->style);
                break;
            default:
                // Box, GizmoAxis 等不支持导出到 DXF
                break;
        }
    }
    
    stream << "0\nENDSEC\n";
}

void DxfHandler::writeEof(QTextStream& stream) {
    stream << "0\nEOF\n";
}

void DxfHandler::writeLine(QTextStream& stream, const Line& line, const Style& style) {
    stream << "0\nLINE\n";
    stream << "8\n0\n";  // 图层 0
    stream << "62\n" << rgbaToAci(style.rgba) << "\n";
    stream << "10\n" << line.p0.x << "\n";
    stream << "20\n" << line.p0.y << "\n";
    stream << "30\n" << line.p0.z << "\n";
    stream << "11\n" << line.p1.x << "\n";
    stream << "21\n" << line.p1.y << "\n";
    stream << "31\n" << line.p1.z << "\n";
}

void DxfHandler::writeCircle(QTextStream& stream, const Circle& circle, const Style& style) {
    stream << "0\nCIRCLE\n";
    stream << "8\n0\n";
    stream << "62\n" << rgbaToAci(style.rgba) << "\n";
    stream << "10\n" << circle.c.x << "\n";
    stream << "20\n" << circle.c.y << "\n";
    stream << "30\n" << circle.c.z << "\n";
    stream << "40\n" << circle.r << "\n";
}

void DxfHandler::writeArc(QTextStream& stream, const Arc& arc, const Style& style) {
    stream << "0\nARC\n";
    stream << "8\n0\n";
    stream << "62\n" << rgbaToAci(style.rgba) << "\n";
    stream << "10\n" << arc.c.x << "\n";
    stream << "20\n" << arc.c.y << "\n";
    stream << "30\n" << arc.c.z << "\n";
    stream << "40\n" << arc.r << "\n";
    // 转换弧度到度
    stream << "50\n" << (arc.a0 * 180.0f / 3.14159265f) << "\n";
    stream << "51\n" << (arc.a1 * 180.0f / 3.14159265f) << "\n";
}

void DxfHandler::writePolyline(QTextStream& stream, const Polyline& poly, const Style& style) {
    if (poly.pts.size() < 2) return;
    
    stream << "0\nLWPOLYLINE\n";
    stream << "8\n0\n";
    stream << "62\n" << rgbaToAci(style.rgba) << "\n";
    stream << "90\n" << poly.pts.size() << "\n";
    stream << "70\n" << (poly.closed ? 1 : 0) << "\n";
    
    for (const auto& pt : poly.pts) {
        stream << "10\n" << pt.x << "\n";
        stream << "20\n" << pt.y << "\n";
        if (pt.z != 0.0f) {
            stream << "30\n" << pt.z << "\n";
        }
    }
}

void DxfHandler::writeRectangle(QTextStream& stream, const Rectangle& rect, const Style& style) {
    // 矩形作为闭合多段线导出
    stream << "0\nLWPOLYLINE\n";
    stream << "8\n0\n";
    stream << "62\n" << rgbaToAci(style.rgba) << "\n";
    stream << "90\n4\n";
    stream << "70\n1\n";  // 闭合
    
    // 四个角点
    stream << "10\n" << rect.p0.x << "\n20\n" << rect.p0.y << "\n";
    stream << "10\n" << rect.p1.x << "\n20\n" << rect.p0.y << "\n";
    stream << "10\n" << rect.p1.x << "\n20\n" << rect.p1.y << "\n";
    stream << "10\n" << rect.p0.x << "\n20\n" << rect.p1.y << "\n";
}

// ============================================================================
// 颜色转换
// ============================================================================

int DxfHandler::rgbaToAci(uint32_t rgba) {
    uint8_t r = (rgba >> 24) & 0xFF;
    uint8_t g = (rgba >> 16) & 0xFF;
    uint8_t b = (rgba >> 8) & 0xFF;
    
    // 白色
    if (r > 230 && g > 230 && b > 230) return 7;
    // 红色
    if (r > 180 && g < 80 && b < 80) return 1;
    // 黄色
    if (r > 180 && g > 180 && b < 80) return 2;
    // 绿色
    if (r < 80 && g > 180 && b < 80) return 3;
    // 青色
    if (r < 80 && g > 180 && b > 180) return 4;
    // 蓝色
    if (r < 80 && g < 80 && b > 180) return 5;
    // 品红
    if (r > 180 && g < 80 && b > 180) return 6;
    
    return 7;  // 默认白色
}

uint32_t DxfHandler::aciToRgba(int aci) {
    switch (aci) {
        case 1: return 0xFF0000FF;  // 红
        case 2: return 0xFFFF00FF;  // 黄
        case 3: return 0x00FF00FF;  // 绿
        case 4: return 0x00FFFFFF;  // 青
        case 5: return 0x0000FFFF;  // 蓝
        case 6: return 0xFF00FFFF;  // 品红
        case 7: return 0xFFFFFFFF;  // 白
        default: return 0xFFFFFFFF;
    }
}
