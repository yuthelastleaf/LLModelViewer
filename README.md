# LLModelViewer
cross platform project for check model

带vcpkg的编译：
cmake -B build -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake

本地编译的环境配置，首先要在c++的配置中，一般本地.vscode目录下面，一个是添加c_cpp_properties.json中的数据，
类似这种,写死的qt路径
```json
{
    "configurations": [
        {
            "name": "Win32",
            "includePath": [
                "${workspaceFolder}/**",
                "${workspaceFolder}/src",
                "${workspaceFolder}/build",
                "C:/Qt/6.9.3/mingw_64/include",
                "C:/Qt/6.9.3/mingw_64/include/QtCore",
                "C:/Qt/6.9.3/mingw_64/include/QtGui",
                "C:/Qt/6.9.3/mingw_64/include/QtWidgets",
                "C:/Qt/6.9.3/mingw_64/include/QtOpenGL",
                "C:/Qt/6.9.3/mingw_64/include/QtOpenGLWidgets",
                "C:/Qt/Tools/mingw1310_64/include",
                "C:/Qt/Tools/mingw1310_64/lib/gcc/x86_64-w64-mingw32/13.1.0/include",
                "C:/Qt/Tools/mingw1310_64/lib/gcc/x86_64-w64-mingw32/13.1.0/include/c++"
            ],
            "defines": [
                "_DEBUG",
                "UNICODE",
                "_UNICODE",
                "WIN32",
                "_WIN32",
                "MINGW",
                "QT_CORE_LIB",
                "QT_GUI_LIB",
                "QT_WIDGETS_LIB",
                "QT_OPENGL_LIB",
                "QT_OPENGLWIDGETS_LIB"
            ],
            "compilerPath": "C:/Qt/Tools/mingw1310_64/bin/g++.exe",
            "cStandard": "c17",
            "cppStandard": "c++17",
            "intelliSenseMode": "windows-gcc-x64",
            "configurationProvider": "ms-vscode.cmake-tools"
        }
    ],
    "version": 4
}
```
然后还要配置cmake的kit，一个是可以通过直接在.vscode目录中创建文件cmake-kits.json,写入类似下面的配置
```json
[
    {
        "name": "Qt 6.9.3 MinGW 64-bit",
        "description": "Qt MinGW 编译器配置",
        "compilers": {
            "C": "C:/Qt/Tools/mingw1310_64/bin/gcc.exe",
            "CXX    ": "C:/Qt/Tools/mingw1310_64/bin/g++.exe"
        },
        "preferredGenerator": {
            "name": "MinGW Makefiles"
        },
        "environmentVariables": {
            "CMAKE_PREFIX_PATH": "C:/Qt/6.9.3/mingw_64",
            "PATH": "C:/Qt/Tools/mingw1310_64/bin;C:/Qt/6.9.3/mingw_64/bin;${env:PATH}"
        },
        "cmakeSettings": {
            "CMAKE_MAKE_PROGRAM": "C:/Qt/Tools/mingw1310_64/bin/mingw32-make.exe",
            "CMAKE_EXPORT_COMPILE_COMMANDS": "ON",
            "CMAKE_BUILD_TYPE": "Debug"
        }
    }
]
```
之后再通过ctrl+shift+P在vs code中选择slect a kit，选中Qt 6.9.3 MinGW 64-bit就行了。
也可以不通过创建文件的方式创建，也是ctrl+shift+P选择edit user-local cmake kits,保证cmake的路径正常，从而保证编译的时候使用正确的编译器。
当然上述都是在windows 系统上的处理。

cmake编译的时候需要设定vcpkg的路径，不然vcpkg的库没有
cmake -B build -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
这样配置了cmake之后，在vscode中才能使用vcpkg下载的库，那么linux也是相同的。

cmake编译的时候，glad库什么的找不到，有时候就是cmakelists中没有写find_package从而还没在vcpkg中找到相关的库，所以才无法链接库的

开发规划：
MVP 里程碑：

v0.1 Viewer：加载 Document，渲染实体；平移/缩放、网格显示、坐标轴。✅
v0.2 选择 & 框选：Pick、SelectionManager、选中高亮。
v0.3 变换 Gizmo：移动工具（单轴/任意）、命令栈 Undo/Redo。
v0.4 绘制工具：Line/Polyline/Circle；捕捉（网格/端点/中点）；预览几何。
v0.5 属性/图层：属性面板编辑颜色线宽；图层显示/锁定；ByLayer。
v0.6 存盘/读盘：JSON *.mcd；工程设置（单位/精度）。
v0.7 几何运算：偏移/倒角/圆角（可先调用简化算法，后续抽象成 Kernel）

std::clamp函数接受三个参数：要限制的值v，下限lo和上限hi。如果v小于lo，则返回lo；如果v大于hi，则返回hi；否则返回v本身。因此，该函数确保返回的值始终在[lo, hi]的范围内。

这是摩尔纹（Moiré Pattern）或锯齿/采样问题，在密集网格渲染中很常见。当网格线密度接近屏幕像素密度时，就会出现这种明暗不均的现象。
这种时候我们就不能直接用opengl的线段绘制了，这样无法处理摩尔纹问题，所以这个时候我们要切换方式，使用着色器来绘制指定的网格内容，这样就能处理这种明暗不均的现象了

问题记录：
glDrawElements 异常通常是以下几个原因导致的。
1. 最可能的原因：Shader 的 uniform 名称不匹配
2. 检查 VAO 是否正确创建
3. 添加 OpenGL 错误检查
4. 检查着色器是否正确加载
当然在我代码中出现的是绘制线条的时候，并没有绑定ibo，所以不能直接使用drawelement，所以要判断是否绑定了ibo再进行绘制，不然就会一场

setMouseTracking在qt中配置后你才能在鼠标不按下的情况下获得鼠标移动事件。

在我们拾取系统的构建中为什么要转到屏幕空间？
视觉一致性：5 像素的拾取范围，无论缩放都是 5 像素，世界坐标的话会受放大以及缩小影响，导致最后的判断
用户体验：符合用户的心理预期（点击的"有效区域"是固定的）
行业标准：所有专业 CAD 软件都这么做
透视适配：自动处理深度变化


================ 以下是点到线段最近点中我们计算的投影参数的大小，主要是点到线段距离的计算，这个还挺重要的。详细的后续更新博客
float t = glm::dot(ap, ab) / abLenSq;
```
**这是什么？**
`t` 是 **投影参数**，表示 `P` 在线段 `AB` 上的投影位置：
```
t = 0.0  →  投影点在 A
t = 0.5  →  投影点在线段中点
t = 1.0  →  投影点在 B
t < 0.0  →  投影点在 A 之前（超出线段）
t > 1.0  →  投影点在 B 之后（超出线段）
```
---
#### 📖 数学推导：为什么是 `dot(ap, ab) / |ab|²`？
**目标**：找到线段上的点 `Q = A + t·AB`，使得 `PQ ⊥ AB`（垂直）
**推导**：
1. 线段上任意点可以表示为：
```
   Q = A + t·AB  (参数方程，t ∈ [0,1])
```
2. 从 `Q` 到 `P` 的向量：
```
   QP = P - Q = P - (A + t·AB) = AP - t·AB
```
3. 垂直条件：`QP ⊥ AB`，即点积为 0：
```
   QP · AB = 0
   (AP - t·AB) · AB = 0
   AP·AB - t·(AB·AB) = 0
   AP·AB = t·|AB|²
   ∴ t = (AP·AB) / |AB|²  ← 这就是公式！


