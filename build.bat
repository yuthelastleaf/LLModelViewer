@echo off
REM ============================================
REM OpenGL Viewer - MinGW 构建脚本
REM ============================================

echo ========================================
echo OpenGL Viewer - MinGW 构建
echo ========================================

REM 配置路径
set QT_PATH=C:/Qt/6.9.3/mingw_64

REM 清理
echo.
echo [1/3] 清理构建目录...
if exist build (
    rmdir /s /q build
    echo 已清理
)

REM 配置 CMake
echo.
echo [2/3] 配置 CMake...
cmake -B build -G "MinGW Makefiles" ^
  -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-mingw-static ^
  -DCMAKE_PREFIX_PATH=%QT_PATH% ^
  -DCMAKE_BUILD_TYPE=Release

if %ERRORLEVEL% neq 0 (
    echo 配置失败！
    pause
    exit /b 1
)

echo 配置成功

REM 编译
echo.
echo [3/3] 编译...
cmake --build build -- -j8

if %ERRORLEVEL% neq 0 (
    echo 编译失败！
    pause
    exit /b 1
)

echo.
echo ========================================
echo 编译成功！
echo 可执行文件: build\bin\OpenGLViewer.exe
echo ========================================

REM 询问是否运行
echo.
set /p run=是否运行程序？(y/n): 
if /i "%run%"=="y" (
    start build\bin\OpenGLViewer.exe
)

pause