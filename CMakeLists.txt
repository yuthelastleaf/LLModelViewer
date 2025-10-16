cmake_minimum_required(VERSION 3.16)
project(OpenGLViewer VERSION 1.0 LANGUAGES CXX)

# ============================================
# vcpkg 配置（在 project() 之前）
# ============================================
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VCPKG_ROOT})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "Vcpkg toolchain file")
        message(STATUS "使用 VCPKG_ROOT: $ENV{VCPKG_ROOT}")
    elseif(EXISTS "D:/GitProject/vcpkg-master/scripts/buildsystems/vcpkg.cmake")
        set(CMAKE_TOOLCHAIN_FILE "D:/GitProject/vcpkg-masterscripts/buildsystems/vcpkg.cmake"
            CACHE STRING "Vcpkg toolchain file")
    endif()
endif()

# ⭐ 关键：检测 Qt 是否为 MinGW 版本，自动设置 triplet
if(NOT DEFINED VCPKG_TARGET_TRIPLET)
    if(EXISTS "C:/Qt/6.9.3/mingw_64" OR MINGW)
        set(VCPKG_TARGET_TRIPLET "x64-mingw-static" CACHE STRING "")
        message(STATUS "自动设置 vcpkg triplet: x64-mingw-static")
    else()
        set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "")
        message(STATUS "自动设置 vcpkg triplet: x64-windows")
    endif()
endif()

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt的MOC、RCC、UIC自动处理
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 查找Qt6库
find_package(Qt6 REQUIRED COMPONENTS 
    Core 
    Gui 
    Widgets 
    OpenGL 
    OpenGLWidgets
)

# 查找OpenGL库
find_package(OpenGL REQUIRED)
# vcpkg 提供的库（可选，找不到会给警告）
# find_package(glad CONFIG)
find_package(assimp CONFIG)
find_package(glm CONFIG)
find_package(Stb)
find_package(fmt CONFIG)

# 主程序和窗口
set(MAIN_SOURCES
    src/main.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/base/opengl/glwidget.h
    src/base/opengl/glwidget.cpp
    src/base/input/inputmanager.h
)

# Demo 系统基础类
set(DEMO_BASE_SOURCES
    src/base/Demo.h
    src/base/Demo.cpp
    src/base/caddemo.h
    src/base/caddemo.cpp
)

# cad 相关类
set(CAD_BASE_SOURCES
    src/cad/data/document.h
    src/cad/data/document.cpp
    src/cad/data/renderer.h
    src/cad/data/renderer.cpp
    src/cad/data/GridAxisHelper.h
    src/cad/data/GridAxisHelper.cpp
)


# 具体 Demo 实现
set(DEMO_IMPL_SOURCES
    src/demo/triangle/TriangleDemo.h
    src/demo/triangle/TriangleDemo.cpp
    # src/TextureDemo.h
    # src/TextureDemo.cpp
    # src/ModelDemo.h
    # src/ModelDemo.cpp
)

# 工具类（相机、光照、网格等）
set(UTILITY_SOURCES
    src/base/camera/Camera.h
    src/base/camera/Camera.cpp
    src/base/light/LightManager.h
    src/base/light/LightManager.cpp
    src/base/util/shader.h
    src/base/util/shader.cpp
)

# UI 控件
set(UI_SOURCES
    src/base/camera/CameraControlPanel.h
    src/base/camera/CameraControlPanel.cpp
)

# 合并所有源文件
set(PROJECT_SOURCES
    ${MAIN_SOURCES}
    ${DEMO_BASE_SOURCES}
    ${CAD_BASE_SOURCES}
    ${DEMO_IMPL_SOURCES}
    ${UTILITY_SOURCES}
    ${UI_SOURCES}
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

# ============================================
# 链接库
# ============================================
target_link_libraries(${PROJECT_NAME} PRIVATE
    # Qt 库
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    
    # OpenGL
    OpenGL::GL
    
    # vcpkg 提供的库
    # glad::glad
    assimp::assimp
    glm::glm
)

# Windows特定设置
if(WIN32)
    # 暂时注释掉，方便调试时看到控制台输出
    # set_target_properties(${PROJECT_NAME} PROPERTIES
    #     WIN32_EXECUTABLE ON
    # )
    
    # 自动部署Qt依赖
    # 方法1: 使用windeployqt（推荐）
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS ${Qt6_DIR}/../../../bin)
    
    if(WINDEPLOYQT_EXECUTABLE)
        message(STATUS "找到 windeployqt: ${WINDEPLOYQT_EXECUTABLE}")
        
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "正在部署Qt依赖..."
            COMMAND ${WINDEPLOYQT_EXECUTABLE} 
                --no-translations           # 不复制翻译文件
                --no-system-d3d-compiler   # 不复制DirectX编译器
                --no-opengl-sw             # 不复制OpenGL软件渲染器
                $<TARGET_FILE:${PROJECT_NAME}>
            COMMENT "Running windeployqt to deploy Qt dependencies"
        )
    else()
        message(WARNING "未找到 windeployqt，将手动复制必要的Qt DLL")
        
        # 方法2: 手动复制关键DLL
        set(QT_DLLS
            Qt6Core
            Qt6Gui
            Qt6Widgets
            Qt6OpenGL
            Qt6OpenGLWidgets
        )
        
        foreach(dll ${QT_DLLS})
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${Qt6_DIR}/../../../bin/${dll}.dll
                    $<TARGET_FILE_DIR:${PROJECT_NAME}>
                COMMENT "Copying ${dll}.dll"
            )
        endforeach()
        
        # 复制platforms插件（qwindows.dll）
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                $<TARGET_FILE_DIR:${PROJECT_NAME}>/platforms
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${Qt6_DIR}/../../../plugins/platforms/qwindows.dll
                $<TARGET_FILE_DIR:${PROJECT_NAME}>/platforms/
            COMMENT "Copying qwindows.dll platform plugin"
        )
        
        # 复制MinGW运行时库
        set(MINGW_DLLS
            libgcc_s_seh-1.dll
            libstdc++-6.dll
            libwinpthread-1.dll
        )
        
        # 尝试从多个可能的位置找MinGW DLL
        find_file(MINGW_PATH libgcc_s_seh-1.dll 
            PATHS 
                ${Qt6_DIR}/../../../bin
                ${Qt6_DIR}/../../../../Tools/mingw*/bin
            NO_DEFAULT_PATH
        )
        
        if(MINGW_PATH)
            get_filename_component(MINGW_BIN_DIR ${MINGW_PATH} DIRECTORY)
            message(STATUS "找到 MinGW bin 目录: ${MINGW_BIN_DIR}")
            
            foreach(dll ${MINGW_DLLS})
                add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        ${MINGW_BIN_DIR}/${dll}
                        $<TARGET_FILE_DIR:${PROJECT_NAME}>
                    COMMENT "Copying ${dll}"
                )
            endforeach()
        else()
            message(WARNING "未找到 MinGW 运行时库，程序可能无法运行")
        endif()
    endif()
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/src/shaders"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
    COMMENT "Copying resource directory"
)

# 打印配置信息
message(STATUS "=== Build Configuration ===")
message(STATUS "CMake version: ${CMAKE_VERSION}")
message(STATUS "Project: ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Qt6 version: ${Qt6_VERSION}")
message(STATUS "Qt6 directory: ${Qt6_DIR}")
message(STATUS "OpenGL found: ${OPENGL_FOUND}")
message(STATUS "Output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "===========================")