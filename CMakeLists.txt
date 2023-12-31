cmake_minimum_required(VERSION 3.5)
project(eIODP)
#project(eIODP LANGUAGES CXX)
# cmake设置
set(CMAKE_BUILD_TYPE "Debug")
#SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -W -g -ggdb")
#SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -W")
SET(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-pthread")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")  #设置c++的编译选项

#QT设置
#set(CMAKE_INCLUDE_CURRENT_DIR ON)
#set(CMAKE_AUTOUIC ON)
#set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTORCC ON)


#find_package(Qt5Core)


# 设置执行文件输出目录
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
# 设置库输出路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)


message("++++++++++++++++++++Start Build++++++++++++++++++++++++++")

# 头文件搜索目录
include_directories(
        ${PROJECT_SOURCE_DIR}/src/ 
        ${PROJECT_SOURCE_DIR}/src/platform/
        ${PROJECT_SOURCE_DIR}/src/platform/iodp_linux/
        ${PROJECT_SOURCE_DIR}/src/platform/iodp_win/
        ${PROJECT_SOURCE_DIR}/src/platform/iodp_qt
)

# 链接库搜索目录
link_directories(${PROJECT_BINARY_DIR}/lib /usr/local/lib /usr/lib)


#设置链接库
if (WIN32)
        link_libraries(wsock32)
        add_library(${PROJECT_NAME}_win STATIC
                src/eiodp.c 
                src/platform/iodp_win/iodp_tcp_win.c
        )
        # 可执行文件
        add_executable(main src/main.c)
        target_link_libraries(main ${PROJECT_NAME}_win)

        # 可执行文件
        add_executable(server_demo demo/win/server_demo.c)
        target_link_libraries(server_demo ${PROJECT_NAME}_win)

        # 可执行文件
        add_executable(client_demo demo/win/client_demo.c)
        target_link_libraries(client_demo ${PROJECT_NAME}_win)

endif()





