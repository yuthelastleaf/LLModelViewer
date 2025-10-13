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

