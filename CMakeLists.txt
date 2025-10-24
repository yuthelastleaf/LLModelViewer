cmake_minimum_required(VERSION 3.16)

# ============================================
# ⚠️ CRITICAL: 这些变量必须在这里设置
# 不能在 if() 条件中设置，必须直接设置
# ============================================
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_C_FLAGS_DEBUG "-g -O0")
# 强制设置 vcpkg triplet（必须在 project() 之前）
set(VCPKG_TARGET_TRIPLET "x64-mingw-static" CACHE STRING "vcpkg triplet" FORCE)


# 强制设置 toolchain file（必须在 project() 之前）
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(EXISTS "D:/GitProject/vcpkg-master/scripts/buildsystems/vcpkg.cmake")
        set(CMAKE_TOOLCHAIN_FILE "D:/GitProject/vcpkg-master/scripts/buildsystems/vcpkg.cmake" 
            CACHE FILEPATH "vcpkg toolchain file" FORCE)
    elseif(DEFINED ENV{VCPKG_ROOT})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" 
            CACHE FILEPATH "vcpkg toolchain file" FORCE)
    else()
        message(FATAL_ERROR "Cannot find vcpkg toolchain file. Please set VCPKG_ROOT environment variable.")
    endif()
endif()

# 调试信息
message(STATUS "========== PRE-PROJECT 配置 ==========")
message(STATUS "CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "VCPKG_TARGET_TRIPLET: ${VCPKG_TARGET_TRIPLET}")
message(STATUS "======================================")

project(OpenGLViewer VERSION 1.0 LANGUAGES CXX)

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

# 打印 POST-PROJECT 诊断信息
message(STATUS "========== POST-PROJECT 诊断 ==========")
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
message(STATUS "VCPKG_INSTALLED_DIR: ${VCPKG_INSTALLED_DIR}")
message(STATUS "VCPKG_TARGET_TRIPLET: ${VCPKG_TARGET_TRIPLET}")

# 手动设置 vcpkg 安装路径（如果 toolchain 没有设置）
if(NOT VCPKG_INSTALLED_DIR)
    if(EXISTS "D:/GitProject/vcpkg-master/installed/${VCPKG_TARGET_TRIPLET}")
        set(VCPKG_INSTALLED_DIR "D:/GitProject/vcpkg-master/installed/${VCPKG_TARGET_TRIPLET}")
        message(STATUS "手动设置 VCPKG_INSTALLED_DIR: ${VCPKG_INSTALLED_DIR}")
        
        # 添加到 CMAKE_PREFIX_PATH
        list(APPEND CMAKE_PREFIX_PATH 
            "${VCPKG_INSTALLED_DIR}"
            "${VCPKG_INSTALLED_DIR}/share"
        )
    elseif(DEFINED ENV{VCPKG_ROOT} AND EXISTS "$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}")
        set(VCPKG_INSTALLED_DIR "$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}")
        message(STATUS "从环境变量设置 VCPKG_INSTALLED_DIR: ${VCPKG_INSTALLED_DIR}")
        
        list(APPEND CMAKE_PREFIX_PATH 
            "${VCPKG_INSTALLED_DIR}"
            "${VCPKG_INSTALLED_DIR}/share"
        )
    endif()
endif()

message(STATUS "最终 CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
message(STATUS "=======================================")

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

# vcpkg 提供的库
message(STATUS "正在查找 vcpkg 包...")

# 方法1：使用 CONFIG 模式（推荐）
find_package(assimp CONFIG)
if(NOT assimp_FOUND)
    # 方法2：手动指定路径
    if(VCPKG_INSTALLED_DIR)
        set(assimp_DIR "${VCPKG_INSTALLED_DIR}/share/assimp")
        message(STATUS "尝试手动路径: ${assimp_DIR}")
        find_package(assimp CONFIG REQUIRED)
    else()
        message(FATAL_ERROR "找不到 assimp 且 VCPKG_INSTALLED_DIR 未设置")
    endif()
endif()
message(STATUS "✓ 找到 assimp: ${assimp_DIR}")

find_package(glm CONFIG)
if(NOT glm_FOUND AND VCPKG_INSTALLED_DIR)
    set(glm_DIR "${VCPKG_INSTALLED_DIR}/share/glm")
    find_package(glm CONFIG REQUIRED)
endif()
message(STATUS "✓ 找到 glm: ${glm_DIR}")

find_package(fmt CONFIG)
if(NOT fmt_FOUND AND VCPKG_INSTALLED_DIR)
    set(fmt_DIR "${VCPKG_INSTALLED_DIR}/share/fmt")
    find_package(fmt CONFIG REQUIRED)
endif()
message(STATUS "✓ 找到 fmt: ${fmt_DIR}")

# Stb 是 header-only 库
find_path(STB_INCLUDE_DIRS "stb_image.h")
if(NOT STB_INCLUDE_DIRS AND VCPKG_INSTALLED_DIR)
    set(STB_INCLUDE_DIRS "${VCPKG_INSTALLED_DIR}/include")
endif()
if(STB_INCLUDE_DIRS)
    message(STATUS "✓ 找到 stb headers: ${STB_INCLUDE_DIRS}")
else()
    message(WARNING "未找到 stb headers")
endif()

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
)

# 工具类（相机、光照、网格等）
set(UTILITY_SOURCES
    src/base/camera/Camera.h
    src/base/camera/Camera.cpp
    src/base/light/LightManager.h
    src/base/light/LightManager.cpp
    src/base/util/shader.h
    src/base/util/shader.cpp
    src/base/util/WorkPlane.h
    src/base/util/WorkPlane.cpp
    src/base/util/RayUtils.h
    src/base/util/RayUtils.cpp
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
# 包含目录
# ============================================
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# 如果找到了 STB，添加其包含目录
if(STB_INCLUDE_DIRS)
    target_include_directories(${PROJECT_NAME} PRIVATE ${STB_INCLUDE_DIRS})
endif()

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
    assimp::assimp
    glm::glm
    fmt::fmt
)

# Windows特定设置
if(WIN32)
    # 自动部署Qt依赖
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS ${Qt6_DIR}/../../../bin)
    
    if(WINDEPLOYQT_EXECUTABLE)
        message(STATUS "找到 windeployqt: ${WINDEPLOYQT_EXECUTABLE}")
        
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "正在部署Qt依赖..."
            COMMAND ${WINDEPLOYQT_EXECUTABLE} 
                --no-translations
                --no-system-d3d-compiler
                --no-opengl-sw
                $<TARGET_FILE:${PROJECT_NAME}>
            COMMENT "Running windeployqt to deploy Qt dependencies"
        )
    else()
        message(WARNING "未找到 windeployqt，将手动复制必要的Qt DLL")
        
        set(QT_DLLS Qt6Core Qt6Gui Qt6Widgets Qt6OpenGL Qt6OpenGLWidgets)
        foreach(dll ${QT_DLLS})
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${Qt6_DIR}/../../../bin/${dll}.dll
                    $<TARGET_FILE_DIR:${PROJECT_NAME}>
            )
        endforeach()
        
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                $<TARGET_FILE_DIR:${PROJECT_NAME}>/platforms
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${Qt6_DIR}/../../../plugins/platforms/qwindows.dll
                $<TARGET_FILE_DIR:${PROJECT_NAME}>/platforms/
        )
        
        find_file(MINGW_PATH libgcc_s_seh-1.dll 
            PATHS ${Qt6_DIR}/../../../bin ${Qt6_DIR}/../../../../Tools/mingw*/bin
            NO_DEFAULT_PATH
        )
        
        if(MINGW_PATH)
            get_filename_component(MINGW_BIN_DIR ${MINGW_PATH} DIRECTORY)
            set(MINGW_DLLS libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll)
            foreach(dll ${MINGW_DLLS})
                add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        ${MINGW_BIN_DIR}/${dll}
                        $<TARGET_FILE_DIR:${PROJECT_NAME}>
                )
            endforeach()
        endif()
    endif()
endif()

# 复制 shaders 目录
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/src/shaders"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
    COMMENT "Copying shader directory"
)

# 打印最终配置信息
message(STATUS "========== Build Configuration ==========")
message(STATUS "Project: ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Qt6 version: ${Qt6_VERSION}")
message(STATUS "==========================================")