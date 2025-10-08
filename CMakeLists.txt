cmake_minimum_required(VERSION 3.16)

if(WIN32 AND NOT CMAKE_TOOLCHAIN_FILE)
    # Qt 和 MinGW 路径
    set(QT_PATH "D:/Qt/6.9.3/mingw_64")
    set(MINGW_PATH "D:/Qt/Tools/mingw1310_64")
    
    # 设置编译器
    set(CMAKE_C_COMPILER "${MINGW_PATH}/bin/gcc.exe" CACHE FILEPATH "C compiler")
    set(CMAKE_CXX_COMPILER "${MINGW_PATH}/bin/g++.exe" CACHE FILEPATH "C++ compiler")
    set(CMAKE_MAKE_PROGRAM "${MINGW_PATH}/bin/mingw32-make.exe" CACHE FILEPATH "Make program")
    
    # 设置 Qt 路径
    set(CMAKE_PREFIX_PATH "${QT_PATH}" CACHE PATH "Qt prefix path")
    
    message(STATUS "Using Qt: ${QT_PATH}")
    message(STATUS "Using MinGW: ${MINGW_PATH}")
endif()

project(QtCMakeDemo VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32 AND NOT DEFINED CMAKE_PREFIX_PATH)
    # 自动检测 Qt MinGW 安装路径
    if(EXISTS "D:/qt/6.9.3/mingw_64")
        set(CMAKE_PREFIX_PATH "D:/qt/6.9.3/mingw_64")
        message(STATUS "Found Qt at: ${CMAKE_PREFIX_PATH}")
    else()
        message(FATAL_ERROR "Qt not found! Please set CMAKE_PREFIX_PATH manually.")
    endif()
endif()

# Qt 自动化工具
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 尝试查找 Qt6，如果找不到则查找 Qt5
find_package(Qt6 COMPONENTS Core Widgets QUIET)

if(NOT Qt6_FOUND)
    find_package(Qt5 5.15 REQUIRED COMPONENTS Core Widgets)
    set(QT_VERSION_MAJOR 5)
    message(STATUS "Using Qt5")
else()
    set(QT_VERSION_MAJOR 6)
    message(STATUS "Using Qt6")
endif()

# 源文件
set(PROJECT_SOURCES
    src/main.cpp
)

# 创建可执行文件
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME}
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(${PROJECT_NAME}
        ${PROJECT_SOURCES}
    )
endif()

# 链接 Qt 库
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
)

# Windows 配置
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# Qt6 需要 finalize
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()

# 安装规则（可选）
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE 
        NAMES windeployqt
        HINTS "${CMAKE_PREFIX_PATH}/bin"
    )
    
    if(WINDEPLOYQT_EXECUTABLE)
        message(STATUS "Found windeployqt: ${WINDEPLOYQT_EXECUTABLE}")
        
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${WINDEPLOYQT_EXECUTABLE}
                --no-translations
                --no-system-d3d-compiler
                --no-opengl-sw
                $<TARGET_FILE:${PROJECT_NAME}>
            COMMENT "Deploying Qt dependencies..."
            VERBATIM
        )
    else()
        message(WARNING "windeployqt not found!")
    endif()
endif()
