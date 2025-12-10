# LLModelViewer - MiniCAD v0.3

跨平台2D/3D CAD查看器，基于Qt + OpenGL。

## 版本特性

### v0.3 - 命令系统与视觉优化
- ✅ **Undo/Redo系统**：完整的命令栈管理
  - 智能合并（500ms内连续操作合并）
  - 内存控制（最大50条命令/100MB）
  - 快捷键：Ctrl+Z撤销，Ctrl+Y重做
- ✅ **移动操作优化**：坐标轴拖拽移动
- ✅ **Hover高亮优化**：圆角边缘、抗锯齿
- ✅ **删除命令**：Delete键删除选中实体

### v0.2 - 选择系统
- 点选/框选实体
- 多选（Shift）和反选（Ctrl）
- Gizmo坐标轴显示

### v0.1 - 基础框架
- 2D/3D视图切换
- 基础图元（线、矩形、圆）
- 网格和坐标轴

## 编译说明

### 依赖
- Qt 6.x
- vcpkg (assimp, glm, fmt, stb)
- MinGW 或 MSVC

### 编译命令
```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
cd build && make -j4
```

### Windows (MinGW)
```powershell
cmake -B build -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=F:/GitProject/vcpkg/scripts/buildsystems/vcpkg.cmake
cd build; mingw32-make -j4
```

## 快捷键

| 快捷键 | 功能 |
|--------|------|
| Ctrl+Z | 撤销 |
| Ctrl+Y | 重做 |
| Delete | 删除选中 |
| Ctrl+S | 保存点 |

## 项目结构

```
src/
├── base/           # 基础框架
│   ├── camera/     # 相机系统
│   ├── opengl/     # OpenGL封装
│   └── caddemo.cpp # CAD主逻辑
├── cad/            # CAD核心
│   ├── command/    # 命令系统
│   ├── data/       # 数据模型
│   ├── selection/  # 选择系统
│   └── transform/  # 变换操作
└── shaders/        # GLSL着色器
```


