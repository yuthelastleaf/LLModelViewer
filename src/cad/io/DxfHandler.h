#pragma once

#include "../data/Document.h"
#include <QString>
#include <QFile>
#include <QTextStream>
#include <vector>
#include <map>

/**
 * DXF文件处理器
 * 支持DXF R12-2018格式的基本图元读写
 */
class DxfHandler {
public:
    // 导入DXF文件到文档
    static bool importFromFile(const QString& filePath, Document* document);
    
    // 导出文档到DXF文件
    static bool exportToFile(const QString& filePath, const Document* document);
    
    // 获取最后的错误信息
    static QString getLastError() { return lastError_; }

private:
    static QString lastError_;
    
    // DXF组码结构
    struct DxfPair {
        int code;
        QString value;
    };
    
    // 图层信息
    struct DxfLayer {
        QString name;
        int color = 7;  // 默认白色
        QString lineType = "CONTINUOUS";
    };
    
    // 解析辅助
    static bool readPair(QTextStream& stream, DxfPair& pair);
    static void skipToSection(QTextStream& stream, const QString& sectionName);
    static void skipToEndSec(QTextStream& stream);
    
    // 实体解析
    static void parseEntitiesSection(QTextStream& stream, Document* document,
                                     const std::map<QString, DxfLayer>& layers);
    static void parseLine(QTextStream& stream, Document* document, const std::map<QString, DxfLayer>& layers);
    static void parseCircle(QTextStream& stream, Document* document, const std::map<QString, DxfLayer>& layers);
    static void parseArc(QTextStream& stream, Document* document, const std::map<QString, DxfLayer>& layers);
    static void parseLwPolyline(QTextStream& stream, Document* document, const std::map<QString, DxfLayer>& layers);
    static void parsePolyline(QTextStream& stream, Document* document, const std::map<QString, DxfLayer>& layers);
    static void parseSpline(QTextStream& stream, Document* document, const std::map<QString, DxfLayer>& layers);
    
    // 图层解析
    static std::map<QString, DxfLayer> parseTablesSection(QTextStream& stream);
    
    // 导出辅助
    static void writeHeader(QTextStream& stream);
    static void writeTables(QTextStream& stream, const Document* document);
    static void writeEntities(QTextStream& stream, const Document* document);
    static void writeEof(QTextStream& stream);
    
    // 实体导出
    static void writeLine(QTextStream& stream, const Line& line, const Style& style);
    static void writeCircle(QTextStream& stream, const Circle& circle, const Style& style);
    static void writeArc(QTextStream& stream, const Arc& arc, const Style& style);
    static void writePolyline(QTextStream& stream, const Polyline& poly, const Style& style);
    static void writeRectangle(QTextStream& stream, const Rectangle& rect, const Style& style);
    
    // 颜色转换 (RGB -> ACI)
    static int rgbaToAci(uint32_t rgba);
    static uint32_t aciToRgba(int aci);
};
