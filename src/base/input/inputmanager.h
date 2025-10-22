#pragma once
#include <QObject>
#include <QSet>
#include <QPointF>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>

class InputManager {
public:
    InputManager() {}

    ~InputManager() {
        currKeys.clear();
        prevKeys.clear();
        currMouseButtons.clear();
        prevMouseButtons.clear();
    }

    // ===== 每帧开始时调用（在 paintGL()/update() 最前面）
    void beginFrame() {
        prevKeys         = currKeys;
        prevMouseButtons = currMouseButtons;
        prevMousePos     = mousePos;              // ✅ 保存上一帧鼠标位置
        mouseDelta       = pendingMouseDelta;
        pendingMouseDelta = {0, 0};
        wheelDelta       = pendingWheelDelta;
        pendingWheelDelta = 0;
    }

    // ===== 键盘状态查询（轮询式）
    bool isKeyDown(int qtKey)       const { return currKeys.contains(qtKey); }
    bool wasKeyPressed(int qtKey)   const { return  currKeys.contains(qtKey) && !prevKeys.contains(qtKey); }
    bool wasKeyReleased(int qtKey)  const { return !currKeys.contains(qtKey) &&  prevKeys.contains(qtKey); }

    // ===== 鼠标状态查询
    bool   isMouseDown(Qt::MouseButton b)      const { return currMouseButtons.contains(b); }
    bool   wasMousePressed(Qt::MouseButton b)  const { return  currMouseButtons.contains(b) && !prevMouseButtons.contains(b); }
    bool   wasMouseReleased(Qt::MouseButton b) const { return !currMouseButtons.contains(b) &&  prevMouseButtons.contains(b); }

    QPointF mousePosition()      const { return mousePos; }
    QPointF prevMousePosition()  const { return prevMousePos; }      // ✅ 新增：获取上一帧位置
    QPointF mouseDeltaPixels()   const { return mouseDelta; }        // 本帧累计移动
    int     wheelDeltaY()        const { return wheelDelta; }        // 本帧累计滚轮

    // ===== 事件入口（在 QWidget/QOpenGLWidget 事件里转发调用）
    void onKeyPress(QKeyEvent* e)    { if (!e->isAutoRepeat())  currKeys.insert(e->key()); }
    void onKeyRelease(QKeyEvent* e)  { if (!e->isAutoRepeat())  currKeys.remove(e->key()); }

    void onMousePress(QMouseEvent* e) { 
        currMouseButtons.insert(e->button()); 
        mousePos = e->position(); 
    }
    
    void onMouseRelease(QMouseEvent* e) { 
        currMouseButtons.remove(e->button()); 
        mousePos = e->position(); 
    }
    
    void onMouseMove(QMouseEvent* e) {
        QPointF p = e->position();
        pendingMouseDelta += (p - mousePos);
        mousePos = p;
    }
    
    void onWheel(QWheelEvent* e) {
        pendingWheelDelta += e->angleDelta().y();
    }

private:
    // 键盘
    QSet<int> currKeys, prevKeys;
    
    // 鼠标
    QSet<Qt::MouseButton> currMouseButtons, prevMouseButtons;
    QPointF mousePos {0, 0};
    QPointF prevMousePos {0, 0};           // ✅ 新增：上一帧鼠标位置
    QPointF pendingMouseDelta {0, 0};      // 事件期间累计
    QPointF mouseDelta {0, 0};             // 本帧实际偏移
    int pendingWheelDelta = 0;
    int wheelDelta = 0;
};