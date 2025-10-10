cmake_minimum_required(VERSION 3.16)
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

# 源文件
set(PROJECT_SOURCES
    src/main.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/glwidget.h
    src/glwidget.cpp
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

# 链接Qt库和OpenGL库
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    OpenGL::GL
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